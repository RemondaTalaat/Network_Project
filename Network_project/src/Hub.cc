//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Hub.h"
#include "Imessage_m.h"

Define_Module(Hub);

//here we don't need to do anything , just get the parameters for any noise to be added
void Hub::initialize()
{
    //initialize sender and receiver with dummy data
    sender = 3;
    receiver = 0;
    // send a self message to start a new session (the previous one should take the specified time here )
    scheduleAt(0, new cMessage("")); // session time = 3 minutes

}

// here we receive a message (data/Ack) apply any noise on it then redirect it to the real receiver.
void Hub::handleMessage(cMessage *msg)
{

    // when receiving a self message allocate new session for 2 random nodes to communicate with each other
    if (msg->isSelfMessage()) {
        
        //sender = uniform(0, par("n"));
        //do {
        //    receiver = uniform(0, par("n"));
        //} while(receiver == sender);

        EV << ". Scheduled a new packet after " << 120 << "s" << endl;
        scheduleAt(simTime() + 120 , new cMessage(""));

     // receiving a message from node
    } else{
        // get the message sender
        int msg_sender = msg->getSenderGateId();

        EV << " the message sender id = "<< msg_sender << endl;
        EV << " the hub sender id = " << sender << endl;
        EV << " the hub receiver id = " << receiver << endl;
        // if a message was received from previous session ignore it
        if (msg_sender != sender && msg_sender != receiver) {
                return;
        }

        // get the message receiver
        int msg_receiver;

        if (msg_sender == sender) {

            msg_receiver = receiver;

        } else {
            msg_receiver = sender;
        }

        // receiving message from the session nodes
        Imessage_Base * rmsg = check_and_cast<Imessage_Base *> (msg);
        //choose what to do with the message
        // options { 0 : modify a bit on it, 1 :delete it, 2: duplicate it, 3: delay it, default case send it normally }
        int choice = uniform(0, 5);



        switch(choice) {
          // modify it before sending
          case 0:
          {
              std::string s = rmsg ->getMessage_payload();
              EV << " message before modification " << s << endl;
              int size = s.size();
              int random_var = uniform(0, size);
              std::bitset<8> bits(s[random_var]);
              EV << bits.to_string() << endl;
              int random_bit = uniform(0,8);
              bits[random_bit] = ! bits[random_bit];
              EV << bits.to_string() << endl;
              s[random_var] = (char)bits.to_ulong();
              Imessage_Base * msg2 = new Imessage_Base("modification");
              std::stringstream ss;
              ss << s;
              msg2->setMessage_payload(ss.str().c_str());

              send(msg2, "outs", msg_receiver );
              EV << " message after modification " << s << endl;

              EV << " Message got modified !" << endl;              // modification code here
              return;
          }
          // delete the message, so it will be lost
          case 1:
          {
              EV << " Message get lost !"<< endl;
              return;
          }
          //  duplicate the message
          case 2:
          {
              Imessage_Base * msg2 = rmsg->dup();
              send(msg2, "outs", msg_receiver );
              EV << " Message got duplicated !" << endl;
              break;
          }
          //  delay the message
          case 3:
          {
              double delay = exponential(0.4);
              sendDelayed(rmsg, delay , "outs", msg_receiver);
              EV << " Message got delayed " << delay << "s" <<endl;
              return;
          }
        }

        send(rmsg, "outs", msg_receiver );

    }


}