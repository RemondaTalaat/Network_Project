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

#include "Node.h"
#include "Imessage_m.h"

Define_Module(Node);

void Node::initialize(){
    /*
    this function generates msgs vector for the node and initializes the window at the beginning of the vector 
    */
    this->generateMsgs();
    // initialize
    this->max_seq = par("max_seq");
    this->S = 0; 
    this->Sf = 0;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    this->R = 0;

    EV << endl << "Node constructed" << endl;

    this->sendMsg();

    
}

void Node::handleMessage(cMessage *msg){
    /*
    this function is responsible for handling different types of received msgs 

    msg types:
    1- self-msg with type = 1 : resend from the beginning if its content not acked yet.
    2- self-msg with type = 2 : send next frame if window is not ended.
    3- self-msg with type = 3 : send ack without piggy-backing if not sent yet.

    4- out-msg with type = 1 : msg carries ack only.
    5- out-msg with type = 2 : msg carries frame only.
    6- out-msg with type = 3 : msg carries ack + frame
    */

    //if self-msg
    if (msg->isSelfMessage()) {
        EV << endl << "self-msg received with type: " << msg->getKind() << endl;
        switch(msg->getKind()) {

            // timeout: check if ack came or not to resend from the beginning of the window
            case 1:
            {
                EV << endl << "This self-msg tells the node to check if the ack of its content came or not, if not, timeout occurs, so the node should resend from the beginning of the window." << endl;
                this->post_timeout_window_resend(msg);
                break;
            }
            
            // send the next frame if there're remaining frames in the window
            case 2:
            {
                EV << endl << "This self-msg tells the node to continue sending if it doesn't get to the end of the window." << endl;
                this->sendMsg();
                break;
            }

            // receeive timeout self-msgs to send ack without piggy backing if not sent yet
            case 3:
            {
                EV << endl << "This self-msg tells the node to check if its content is sent as ack or not, if not, send without piggy-backing" << endl;
                this->post_timeout_send_ack_only(msg);
                break;
            }

        }

    }

    // if out msg arrived
    else {
        EV << endl << "out-msg received with type: " << msg->getKind() << endl;
        switch(msg->getKind()) {

            // ack only
            case 1:
            {
                EV << endl << "This out-msg carries ack only." << endl;
                this->post_receive_ack(msg);
                break;
            }
            
            // frame only
            case 2:
            {
                EV << endl << "This out-msg carries frame only." << endl;
                this->post_receive_frame(msg);
                break;
            }

            // frame + ack 
            case 3:
            {
                EV << endl << "This out-msg carries ack + frame (piggy-backing)." << endl;
                this->post_receive_ack(msg);
                this->post_receive_frame(msg);
                break;
            }

        }
    
    }

}

void Node::generateMsgs(){
    /*
    this function reads a random file as its msg vector
    */
    // TODO: read from random file
    this->msgs.push_back("hi there baby!");
    this->msgs.push_back("hello!");
    this->msgs.push_back("how are you?");
    this->msgs.push_back("I love you");
    this->msgs.push_back("hi1");
    this->msgs.push_back("hi2");
    this->msgs.push_back("hi3");
    this->msgs.push_back("hi4");
    this->msgs.push_back("hi5");

}

void Node::sendMsg(){
    /*
    this function is responsible for
    1- sending frames with acks if piggy-backing is available.
    2- sending self-msg to resend the window if the sent frame is not acked after a timeout.
    3- sending self-msg to send the next frame if the window is not ended.
    */
    // check if I reached the end of the window
    if (this->S <= this->Sl){
        EV << endl << "I can send the next frame as window is not ended." << endl;
        // send msg referred by S
        const char* msg_payload = this->msgs[this->S].c_str();
        Imessage_Base * msg = new Imessage_Base(msg_payload);
        msg ->setSequence_number(this->S % (this->max_seq + 1));
        msg ->setMessage_payload(msg_payload);

        // piggy backing frame + ack
        if (!this->acks_to_be_sent.empty()){
            EV << endl << "GREAT, there're acks available to be piggy backed with the frame. Bandwidth is SAVED!" << endl;
            int ack = this->acks_to_be_sent.front();
            msg->setAcknowledge(ack);
            msg->setKind(3);

            // delete the sent acknowledge from the buffering queue
            this->acks_to_be_sent.pop();
        }

        // frame only
        else{
            EV << endl << "No available acks to be piggy backed, send the frame alone." << endl;
            msg->setKind(2);
        }
        
        // msg is ready -> send it
        send(msg,"out");

        EV << endl << "I sent the frame of index " << this->S << " ,whose content is " << this->msgs[this->S] << endl;

        EV << endl << "send myself a msg to resend starting from this current frame, if it isn't acked" << endl;
        // send self-msg including sent_msg_seq_num to myself as a timer to re-send if it's not acked
        cMessage * time_out_self_msg = new cMessage(std::to_string(this->S).c_str());
        time_out_self_msg->setKind(1);
        scheduleAt(simTime() + par("window_resend_timeout") , time_out_self_msg);

        // increment S
        this->S++;
    }
    
    // send self-msg to send a new frame after a timeout
    cMessage * send_next_self_msg = new cMessage("");
    send_next_self_msg->setKind(2);
    scheduleAt(simTime() + 1 , send_next_self_msg);

}

void Node::post_receive_ack(cMessage *msg){
    /*
    this function slide the window using cumulative acknowledge if there is an ack received.
    */
    EV << endl << "Ack on frame with seq_num = " << ((Imessage_Base *)msg)->getAcknowledge() << " is received, slide the window now!" << endl;
    EV << endl << "Window before sliding has Sf = " << this->Sf << " and Sl = " << this->Sl << endl;
    // slide the window
    while(this->Sf % (this->max_seq + 1) != (((Imessage_Base *)msg)->getAcknowledge())){
        this->Sf ++;
    }
    this->Sf ++;
    this->Sl = std::min(this->Sf + max_seq - 1, int(this->msgs.size())-1);
    EV << endl << "Window after sliding has Sf = " << this->Sf << " and Sl = " << this->Sl << endl;
}

void Node::post_receive_frame(cMessage *msg){
    /*
    this function is called after receiving a frame and responsible for checking if the received frame is in-order and correct without errors

    - if yes
        1- add the ack in the buffer to be acked as soon as possible (with piggy-backing if available) 
        2- send self-msg as a timeout to send the ack alone without piggy backing if it is not sent yet (i.e. still in the buffer)
        3- increment the expected next frame to be received

    - if not : SILENT
    */
    // check inorder receive
    int received_frame_seq_num = ((Imessage_Base *)msg)->getSequence_number();
    EV << endl << "a frame with seq_num = " << received_frame_seq_num << " is received" << endl;

    if (received_frame_seq_num == this->R){
        // TODO: check errors
        EV << endl << "the frame is in-order" << endl;
        // add the ack to the buffer
        this->acks_to_be_sent.push(received_frame_seq_num);
        
        // add a self-msg as a timeout that after it the ack will be sent alone -> no piggy backing
        cMessage * ack_time_out_self_msg = new cMessage(std::to_string(received_frame_seq_num).c_str());
        ack_time_out_self_msg->setKind(3);
        scheduleAt(simTime() + par("ack_send_timeout") , ack_time_out_self_msg);  

        // increase next frame expected to be received (circular increment)  
        this->R++;
        this->R = this->R % (this->max_seq + 1);
        return;
    }
    EV << endl << "the frame is out-of-order, I am silent!" << endl;
}

void Node::post_timeout_window_resend(cMessage *msg){
    /*
    this function is called if a self-msg with type = 1 is received
    resend from the beginning of the window if the msg's content is not acked yet.
    */
    bool compare = strcmp(msg->getName(), std::to_string(this->Sf).c_str());
    if(compare == 0) {
        EV << endl << "No ack came, TIMEOUT!!!, RESEND!!!" << endl;
        // timeout: no ack came -> resend
        this->S = Sf;
    }
}

// no piggy backing
void Node::post_timeout_send_ack_only(cMessage *msg){
    /*
    this function is called if a self-msg with type = 3 is received
    send ack without piggy-backing if not sent yet.
    */
    // check if the received frame to be acknowledged is the buffered frame to be acknowledged (i.e. not sent yet)
    bool compare = strcmp(msg->getName(), std::to_string(this->acks_to_be_sent.front()).c_str());
    
    if(compare == 0) {
        // ack is not sent -> send it without piggy backing
        EV << endl << "Ack of frame with seq_num = " << this->acks_to_be_sent.front() << " wasn't piggy-backed ever and TIMEOUT arose. SEND it alone! Bandwidth wasn't SAVED!" << endl;
        // send seq number of the frame to be acknowledged (top of the queue)
        int ack = this->acks_to_be_sent.front();
        const char* msg_payload = std::to_string(ack).c_str();
        Imessage_Base * msg = new Imessage_Base(msg_payload);
        msg->setMessage_payload(msg_payload);
        msg->setAcknowledge(ack);
        msg->setKind(1);
        send(msg,"out"); 
            
        // delete the sent acknowledge from the buffering queue
        this->acks_to_be_sent.pop();
        
    } 
}
