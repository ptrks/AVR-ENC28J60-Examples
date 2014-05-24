UDP Client Send
===================

This basic example shows you how to send UDP data to a remote host using a software based delay loop. 

Here are a few notes to help you navigate:
* `mac_address` - Hardware address of your device. It must be unique.
* `remote_host` - IP address that you want to send data to.
* `gateway` - Default gateway on your network. 
* `local_port` - Port that responses would be sent to. In this example we only send data.
* `remote_port` - Port on the `remote_host` that data is sent to.