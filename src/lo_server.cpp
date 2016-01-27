#include <iostream>
#include <fstream>
#include <uv.h>
#include <assert.h>
#include <cstring>
#include "lo_server.h"
using namespace std;


const char * http_request;
const char * http_response;
uv_tcp_t _server;
uv_tcp_t _client;
uv_loop_t * _loop;
/*
const char* http_response = "HTTP/1.1 200 OK\r\n"
    "Content-Type:text/html;charset=utf-8\r\n"
        "Content-Length:23\r\n"
            "\r\n"
               "<h3>Link to files:</h3>";
*/

void lo::tinyweb_start(uv_loop_t* loop, const char* ip, int port) {
        sockaddr_in addr;
        uv_ip4_addr(ip, port, &addr);
        _loop = loop;
        uv_tcp_init(_loop, &_server);
        uv_tcp_bind(&_server, (const struct sockaddr*) &addr, 0);
        uv_listen((uv_stream_t*)&_server, 8, tinyweb_on_connection);
}

void lo::tinyweb_close_client(uv_stream_t * client)
{
	uv_close((uv_handle_t *)client,after_uv_close);
}

void lo::after_uv_close(uv_handle_t* handle) {
        delete handle; // uv_tcp_t* client, see tinyweb_on_connection()
}

void lo::on_uv_alloc(uv_handle_t * handle,size_t suggested_size,uv_buf_t *buf)
{
        char *base=new char[suggested_size];
        *buf=uv_buf_init(base,suggested_size);
}

void lo::after_uv_write(uv_write_t* w, int status) {
        uv_close((uv_handle_t*)w->handle, after_uv_close); // close client
}

void lo::on_uv_read(uv_stream_t *client,ssize_t nread,const uv_buf_t *buf)
{
	http_request=buf->base;
        if(nread>0)
        {
		http_response=get_response(http_request);
	    	write_uv_data(client,http_response,-1,0);
        }
	else if(nread==-1)
	{
		tinyweb_close_client(client);
	}
}

const char * lo::get_response(const char *request)
{
	string method,str_response="",opline,line;
	const char * path;
	int index=0;
	string req_uri;
	//method
	if(request[index]=='G'&&request[index+1]=='E'&&request[index+2]=='T') //GET method
	{
	    index+=3;
	    method="GET";
	}
	else if(request[index]=='P'&&request[index]=='O'&&request[index]=='S'&&request[index]=='T') //post method
	{   
	    method="POST";   
	    index+=4;
	}
	index++; //space 
	//uri
	while(request[index]!=' ')
	{
	    req_uri+=request[index];
	    index++;
	}
	index++; //space
	//version

	/*
	    analyze requests
	*/
	if(method=="GET")
	{
	    fstream file;
	    path=("../test"+req_uri).c_str();
	    file.open(path);
	    if(!file)
	    {
		str_response =str_response+ "HTTP/1.1 200 OK\r\n"+"Content-Type:text/html;charset=utf-8\r\n"+"Content-Length:1\r\n"+"\r\n"+"2";
            	http_response=str_response.c_str();
	    }
	    else
            {
		//read request page
		ifstream file;
		file.open(path);
		while(getline(file,opline))
			line+=opline;
		file.close();
	    	str_response =str_response+ "HTTP/1.1 200 OK\r\n"+"Content-Type:text/html;charset=utf-8\r\n"+"Content-Length:91\r\n"+"\r\n"+line;
	    	http_response=str_response.c_str();
  	    }
	}
	return http_response;
}

void lo::write_uv_data(uv_stream_t* stream, const char* data, unsigned int len, int need_copy_data) {
        uv_buf_t buf;
        uv_write_t* w=new uv_write_t;
        char* newdata =strdup(data);
        if(data == NULL || len == 0) return;
        if(len ==(unsigned int)-1)
            len = strlen(data);
        buf = uv_buf_init(newdata, len);
        w->data = newdata;
        uv_write(w, stream, &buf, 1, after_uv_write); // free w and w->data in after_uv_write()

}

void lo::tinyweb_on_connection(uv_stream_t* server, int status) {
    assert(server == (uv_stream_t*)&_server);
    if(status == 0) {
        uv_tcp_t* client = new uv_tcp_t;
        uv_tcp_init(_loop, client);
        uv_accept((uv_stream_t*)&_server, (uv_stream_t*)client);
        uv_read_start((uv_stream_t *)client,on_uv_alloc,on_uv_read);
       //write_uv_data((uv_stream_t*)client, http_respone, -1, 0);
       //close client after uv_write, and free it in after_uv_close()
        }

}
