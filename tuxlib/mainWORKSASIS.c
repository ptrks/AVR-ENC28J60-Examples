/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * ETH alarm system
 * UDP and HTTP interface 
 *
 * Chip type           : Atmega168 or Atmega328 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"
#include "enc28j60.h"
#include "timeout.h"
#include "net.h"

//---------------- start of modify lines --------
// please modify the following lines. mac and ip have to be unique
// in your network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x29};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t myip[4] = {192,168,1,144};
// listen port for tcp/www:
#define MYWWWPORT 80
// listen port for udp
#define MYUDPPORT 1200
// the password string (only a-z,0-9,_ characters):
static char password[10]="sharedsec"; // must not be longer than 9 char
// MYNAME_LEN must be smaller than gStrbuf (below):
#define STR_BUFFER_SIZE 30
#define MYNAME_LEN STR_BUFFER_SIZE-14
static char myname[MYNAME_LEN+1]="section-42";
// IP address of the alarm server to contact. The server we send the UDP to
static uint8_t udpsrvip[4] = {192,168,1,155};
static uint16_t udpsrvport=9600;
static uint8_t data[18] = {0x80,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x7a,0x01,0x01,0x00,0xcc,0xcc,0xcc,0x00,0x01};

// Default gateway. The ip address of your DSL router. It can be set to the same as
// udpsrvip the case where there is no default GW/router to access the
// server (=server is on the same lan as this host)
static uint8_t gwip[4] = {192,168,1,1};
//
// the alarm contact is PD0.
// ALCONTACTCLOSE=1 means: alarm when contact between GND and PD0 closed 
#define ALCONTACTCLOSE 1
// alarm when contact between PD0 and GND is open
//#define ALCONTACTCLOSE 0
//---------------- end of modify lines --------
//
#define BUFFER_SIZE 650
static uint8_t buf[BUFFER_SIZE+1];
static uint16_t gPlen;
static char gStrbuf[STR_BUFFER_SIZE+1];
static uint8_t alarmOn=1; // alarm system is on or off
static uint8_t lastAlarm=0; // indicates if we had an alarm or not
// timing:
static volatile uint8_t cnt2step=0;
static volatile uint8_t gSec=0;
static volatile uint16_t gMin=0; // alarm time min

// set output to VCC, red LED off
#define LEDOFF PORTB|=(1<<PORTB1)
// set output to GND, red LED on
#define LEDON PORTB&=~(1<<PORTB1)
// to test the state of the LED
#define LEDISOFF PORTB&(1<<PORTB1)
// 
uint8_t verify_password(char *str)
{
        // the first characters of the received string are
        // a simple password/cookie:
        if (strncmp(password,str,strlen(password))==0){
                return(1);
        }
        return(0);
}

uint16_t http200ok(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t print_webpage_config(void)
{
        uint16_t plen;
        plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=/>[home]</a>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<h2>Alarm config</h2><pre>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<form action=/u method=get>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("Enabled:<input type=checkbox value=1 name=ae "));
        if (alarmOn){
                plen=fill_tcp_data_p(buf,plen,PSTR("checked"));
        }
        plen=fill_tcp_data_p(buf,plen,PSTR(">\nName:   <input type=text name=n value=\""));
        plen=fill_tcp_data(buf,plen,myname);
        plen=fill_tcp_data_p(buf,plen,PSTR("\">\nSendto: ip=<input type=text name=di value="));
        mk_net_str(gStrbuf,udpsrvip,4,'.',10);
        plen=fill_tcp_data(buf,plen,gStrbuf);
        plen=fill_tcp_data_p(buf,plen,PSTR("> port=<input type=text name=dp size=4 value="));
        itoa(udpsrvport,gStrbuf,10); 
        plen=fill_tcp_data(buf,plen,gStrbuf);
        plen=fill_tcp_data_p(buf,plen,PSTR("> gwip=<input type=text name=gi value="));
        mk_net_str(gStrbuf,gwip,4,'.',10);
        plen=fill_tcp_data(buf,plen,gStrbuf);
        plen=fill_tcp_data_p(buf,plen,PSTR(">\nPasswd: <input type=password name=pw>\n<input type=submit value=change></form>\n<hr>"));
        return(plen);
}

// main web page
uint16_t print_webpage(void)
{
        uint16_t plen;
        plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=/c>[config]</a> <a href=./>[refresh]</a>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<h2>Alarm: "));
        plen=fill_tcp_data(buf,plen,myname);
        plen=fill_tcp_data_p(buf,plen,PSTR("</h2><pre>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("Last alarm:\n"));
        if (lastAlarm){
                if (gMin>59){
                        itoa(gMin/60,gStrbuf,10); // convert integer to string
                        plen=fill_tcp_data(buf,plen,gStrbuf);
                        plen=fill_tcp_data_p(buf,plen,PSTR("hours and "));
                }
                itoa(gMin%60,gStrbuf,10); // convert integer to string
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("min ago"));
        }else{
                plen=fill_tcp_data_p(buf,plen,PSTR("none in last 14 days"));
        }
        plen=fill_tcp_data_p(buf,plen,PSTR("\n</pre><hr>\n"));
        return(plen);
}

void data2eeprom(void)
{
        eeprom_write_byte((uint8_t *)40,19); // magic number
        eeprom_write_block((uint8_t *)gwip,(void *)41,sizeof(gwip));
        eeprom_write_block((uint8_t *)udpsrvip,(void *)45,sizeof(udpsrvip));
        eeprom_write_word((void *)49,udpsrvport);
        eeprom_write_byte((uint8_t *)51,alarmOn);
        eeprom_write_block((uint8_t *)myname,(void *)52,sizeof(myname));
}

void eeprom2data(void)
{
        if (eeprom_read_byte((uint8_t *)40) == 19){
                // ok magic number matches accept values
                eeprom_read_block((uint8_t *)gwip,(void *)41,sizeof(gwip));
                eeprom_read_block((uint8_t *)udpsrvip,(void *)45,sizeof(udpsrvip));
                udpsrvport=eeprom_read_word((void *)49);
                alarmOn=eeprom_read_byte((uint8_t *)51);
                eeprom_read_block((char *)myname,(void *)52,sizeof(myname));
        }
}

// analyse the url given
//                The string passed to this function will look like this:
//                ?s=1 HTTP/1.....
//                We start after the first slash ("/" already removed)
int8_t analyse_get_url(char *str)
{
        // the first slash:
        if (*str == 'c'){
                // configpage:
                gPlen=print_webpage_config();
                return(10);
        }
        if (*str == 'u'){
                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"pw")){
                        urldecode(gStrbuf);
                        if (verify_password(gStrbuf)){
                                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"n")){
                                        urldecode(gStrbuf);
                                        gStrbuf[MYNAME_LEN]='\0';
                                        strcpy(myname,gStrbuf);
                                }
                                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"ae")){
                                        alarmOn=1;
                                }else{
                                        alarmOn=0;
                                }
                                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"di")){
                                        urldecode(gStrbuf);
                                        if (parse_ip(udpsrvip,gStrbuf)!=0){
                                                return(-2);
                                        }
                                }
                                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"dp")){
                                        gStrbuf[4]='\0';
                                        udpsrvport=atoi(gStrbuf);
                                }
                                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"gi")){
                                        urldecode(gStrbuf);
                                        if (parse_ip(gwip,gStrbuf)!=0){
                                                return(-2);
                                        }
                                }
                                data2eeprom();
                                gPlen=http200ok();
                                gPlen=fill_tcp_data_p(buf,gPlen,PSTR("<a href=/>[home]</a>"));
                                gPlen=fill_tcp_data_p(buf,gPlen,PSTR("<h2>OK</h2>"));
                                return(10);
                        }
                }
                return(-1);
        }
        return(0);
}

// called when TCNT2==OCR2A
// that is in 50Hz intervals
// This is used as a "clock" to store the data
ISR(TIMER2_COMPA_vect){
        cnt2step++;
        if (cnt2step>49){ 
                gSec++;
                cnt2step=0;
        }
        if (gSec>59){ 
                gSec=0;
                if (lastAlarm){
                        gMin++;
                        // 14 days is the limit:
                        if (gMin>60*24*14){
                                gMin=0;
                                lastAlarm=0; // reset lastAlarm
                        }
                }
        }
}

/* setup timer T2 as an interrupt generating time base.
* You must call once sei() in the main program */
void init_cnt2(void)
{
        cnt2step=0;
        PRR&=~(1<<PRTIM2); // write power reduction register to zero
        TIMSK2=(1<<OCIE2A); // compare match on OCR2A
        TCNT2=0;  // init counter
        OCR2A=244; // value to compare against
        TCCR2A=(1<<WGM21); // do not change any output pin, clear at compare match
        // divide clock by 1024: 12.5MHz/1024=12207.0313 Hz
        TCCR2B=(1<<CS22)|(1<<CS21)|(1<<CS20); // clock divider, start counter
        // 12207.0313 / 244= 50.0288
}

int main(void){
        uint16_t dat_p;
        uint16_t contact_debounce=0;
#define DEBOUNCECOUNT 0x1FFF
        int8_t cmd;
        uint8_t payloadlen=0;
        char cmdval;
        
        // set the clock speed to "no pre-scaler" (8MHz with internal osc or 
        // full external speed)
        // set the clock prescaler. First write CLKPCE to enable setting of clock the
        // next four instructions.
        CLKPR=(1<<CLKPCE); // change enable
        CLKPR=0; // "no pre-scaler"
        _delay_loop_1(0); // 60us

        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_loop_1(0); // 60us
        enc28j60PhyWrite(PHLCON,0x476);
        //
        eeprom2data();
        // time keeping
        init_cnt2();
        sei();
        //
        DDRB|= (1<<DDB1); // enable PB1, LED as output 
        // enable PD0 as input for the alarms system:
        DDRD&= ~(1<<DDD0);
        PORTD|= (1<<PIND0); // internal pullup resistor on, jumper goes to ground
        //init the web server ethernet/ip layer:
        init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);
        client_set_gwip(gwip);  // e.g internal IP of dsl router

        while(1){

                // handle ping and wait for a tcp/udp packet
                gPlen=enc28j60PacketReceive(BUFFER_SIZE, buf);
                dat_p=packetloop_icmp_tcp(buf,gPlen);
                _delay_ms(2000);
                send_udp(buf,data,sizeof(data),MYUDPPORT, udpsrvip, udpsrvport);

                _delay_ms(2000);





                // if(dat_p==0){
                //         // no pending packet
                //         if (gPlen==0){
                //                 send_udp(buf,data,(strlen(UDP_DATA_P)+strlen(data)),MYUDPPORT, udpsrvip, udpsrvport);
                //                 _delay_ms(2000);
                //                 continue;
                //         }
                //         // pending packet, check if udp otherwise continue
                //         //send_udp(buf,data,(strlen(UDP_DATA_P)+strlen(data)),MYUDPPORT, udpsrvip, udpsrvport);
                //         //_delay_ms(1000);
                //         LEDON;
                //         continue;
                // }
        }
         

        return (0);
}
