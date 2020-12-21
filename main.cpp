///This program is free software: you can redistribute it and/or modify
///it under the terms of the GNU General Public License as published by
///the Free Software Foundation, either version 3 of the License, or
///(at your option) any later version.

///This program is distributed in the hope that it will be useful,
///but WITHOUT ANY WARRANTY; without even the implied warranty of
///MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///GNU General Public License for more details.

///see <https://www.gnu.org/licenses/> for more info related to the
///GNU General Public License.


#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <amqp.h>
#include <amqp_ssl_socket.h>
#include <assert.h>
#include "CPPAMQP.h"

using namespace std;

int main()
{
    arax::CPPAMQP rmq{"192.168.1.11", 5672, 500000};
    rmq.connect_normal("/", "user", "password");
	//rmq.connect_ssl("/","user", "pass","/dir/keys/CSR.pem","/dir/keys/cert.pem","/dir/keys/key.pem");

    rmq.listen_to_queue(1,"queuenqme");
    struct timeval tval{};
    tval.tv_usec = 5000000;
    rmq.send_stream(cin ,1,"exchange","routing_key",0,0);
    //auto str_vec = o.receive_string(1, &tval);
    //for (auto [exchange, routing_key, message]: str_vec)
      //cout<<exchange<<" - "<<routing_key<<" - "<<message<<endl;
    return 0;
}
