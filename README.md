# CPP_AMQP
It is basically just a simple C++ wrapper written by me(ardeshiri) around rabbitmq-c library by Alan Antonuk at (alanxz: https://github.com/alanxz/rabbitmq-c).
This simple wrapper is by no means neither complete, nor will be. It is developed to be used in a hobbyist IOT project and will change and evolve during time with no warning.
It needs rabbitmq-c library installed. Use -lrabbitmq to link properly.

tutorials:

1_constructor:

arax::CPPAMQP(std::string h_name, int prt, int socket_create_timeout); // timeout in ms

example:

arax::CPPAMQP rmq{"localhost", 5672, 500000};


2_copy constructor and copy assignment op are deleted:

CPPAMQP(const CPPAMQP&) = delete;

CPPAMQP& operator=(const CPPAMQP&) = delete;


3_CPPAMQP objects are movable. 

example:

auto newrmq = std::move(currentinstance);

CPPAMQP newrmq {std::move(currentinstance)};


4_no need to close anything, all is done using RAII;


5_connect:

void arax::CPPAMQP::connect_normal(std::string vhost, std::string user_name, std::string password, int channel_max=0,
	       	     		    int frame_max=131072, int heartbeat=0, amqp_sasl_method_enum sasl_method=AMQP_SASL_METHOD_PLAIN)

void arax::CPPAMQP::connect_ssl(std::string vhost, std::string user_name, std::string password,const char *cacert,const char *cert, const char *key, int channel_max = 0,
		     			 int frame_max = 131072, int heartbeat=0, amqp_sasl_method_enum sasl_method=AMQP_SASL_METHOD_PLAIN)

example:

rmq.connect_normal("/", "user", "password");

rmq.connect_ssl("/","user", "password-w2suHVB8UilCYXS5fsnZgaVp","/dir/keys/CSR.pem","/dir/keys/cert.pem","/dir/keys/key.pem");


6_listening to a queue:

void listen_to_queue(amqp_channel_t channel,const std::string& queue_name,const amqp_bytes_t& consumer_tag = amqp_empty_bytes, amqp_boolean_t no_local = 0,
		      	     amqp_boolean_t no_ack = 0, amqp_boolean_t exclusive = 0,const amqp_table_t& arguments = amqp_empty_table );

example:

rmq.listen_to_queue(1,"mybuck");


7_receiving:

std::vector<amqp_envlp_t> receive( const int max_envs, struct timeval *tvp);

std::vector<std::tuple<std::string,std::string,std::string>> receive_string( const int max_envs, struct timeval *tvp);

void receive_stream(std::ostream& os, struct timeval *tvp, const std::string = "", const std::string = "");

example:

struct timeval tval{};

tval.tv_usec = 5000000;

auto ret_vec = rmq.receive_string(1, &tval);

rmq.receive_stream(cout, &tval);


8_sending:

void send(const std::vector< std::vector<unsigned char>>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
		         const amqp_boolean_t mandatory,const amqp_boolean_t immediate);

void send_string(const std::vector<std::string>& input_v, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
		        const amqp_boolean_t mandatory,const amqp_boolean_t immediate);

void send_stream(std::istream& is, const amqp_channel_t channel,const std::string& exchange,const std::string& routing_key,
		        const amqp_boolean_t mandatory,const amqp_boolean_t immediate);

example:

rmq.send_string(vector_of_strings, 1, "exchange", "routing_key", 0, 0);

rmq.send_stream(cin, 1,"exchange","routing_key", 0, 0);


a simple complete program is provided in main.


This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

see <https://www.gnu.org/licenses/> for more info related to the 
GNU General Public License. 
