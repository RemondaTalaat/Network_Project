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

Define_Module(Hub);
/**
 * function used to generate the sessions table which consists of pairs of nodes randomly generated.
 */
void Hub::generatePairs()
{
    int node1,node2;
    // loop over the nodes square times
    for(int i =0; i < this->n*this->n; ++i)
    {
        // pick random node to be the first node
        node1 = uniform(0, this->n);
        // push it in senders vector
        this->senders.push_back(node1);
        // pick random node not equal the first one to be the second node
        do
        {
            node2 = uniform(0, this->n);
        }
        while(node2 == node1);
        // push it to the receivers vector
        this->receivers.push_back(node2);
    }
    // Debugging prints
    /*for(int i = 0; i < this->n*this->n; ++i) 
    {
        EV <<  "senders[" << i << "] = " << this->senders[i] <<endl;
        EV << " Receivers[" << i << "] = " << this->receivers[i] <<endl;
    }*/
}
/**
 * function used to initialize the hub data members.
 */
void Hub::initialize()
{
    // read the number of nodes
    this->n = par("n");

    // initially not refer to the table
    this->indexer = -1;

    //generate the nodes pairs table
    generatePairs();

    // initialize statistics gathering variables
    initializeStats();

    // initialize the first session nodes to the first pair in the table
    this->sender = this->senders[0];
    this->receiver = this->receivers[0];

    // send a start session message to start a new session
    this->start_session = new cMessage("");
    this->start_session->setKind(1);
    scheduleAt(simTime() + par("session_time"), this->start_session); // session time

    // send a statistical message to print statistics of the system every 3 minutes
    this->print_stats = new cMessage("");
    this->print_stats->setKind(2);
    scheduleAt(simTime() + par("stats_time"), this->print_stats); // stats print time
}
/**
 * start new session by selecting the ordered pairs from table and send them messages to start.
 */
void Hub::startSession()
{
    // advance the indexer to schedule the next pairs
    this->indexer ++;
    // when reaching end of the table reset the indexer
    if(this->indexer == this->senders.size())
    {
        this->indexer = 0;
    }
    // set the session nodes
    this->sender = this->senders[this->indexer];
    this->receiver = this->receivers[this->indexer];
    // send messages to notify the nodes the session started
    cMessage* msg = new cMessage("sender");
    msg ->setKind(4);
    send(msg, "outs", sender);
    cMessage* msg2 = new cMessage("receiver");
    msg2 ->setKind(4);
    send(msg2, "outs", receiver);
    // schedule the next session if none of the nodes finishes during the session time
    EV << ". Scheduled a new session after " << par("session_time").doubleValue()  << "s" << endl;
    scheduleAt(simTime() + par("session_time") , this->start_session);
}
/**
 * mimic the channel effect by choosing an action to do with the message, lose it or delay it or neither of them.
 * then modify a random bit in the message if the probability of modification > 50% ( can be modified from monetpp.ini file )
 * then duplicate it if the probability of duplication > 50% ( can be modified from omnetpp.ini file )
 @param msg : the message received as is.
 @return int represent the action to be applied to the message as following :
     0 -> message lost.
     1 -> delayed.
     2 -> delayed + duplicated.
     3 -> duplicated.
     4 -> send it as is.
*/
int Hub::applyNoise(Imessage_Base * msg)
{
    // generate random action from inverted actions ( losing , delaying , none )
    int choice =uniform(0,3);  // 0 -> lose , 1 -> do nothing , 2 -> delayed
    // in case of 0 means lose the message so return 0 so the message deleted
    if (choice == 0)
    {
        EV << "Message lost !" <<endl;
        return 0;
    }
    // calculate the probability to modify a message
    int prob_modify  = uniform(0,1)*10;
    // in case of message modification
    if(prob_modify > par("prob_modification").doubleValue())
    {
        // read the message payload
        std::string s = msg ->getMessage_payload();
        // print payload before modification to assure modification happens
        EV << " message before modification " << s << endl;
        // pick random char from the message payload to modify it
        int size = s.size();
        int random_var = uniform(0, size);
        // convert that char to its binary representation
        std::bitset<8> bits(s[random_var]);
        // pick random bit in that char to modify
        EV << bits.to_string() << endl;
        int random_bit = uniform(0,8);
        //invert the selected bit
        bits[random_bit] = ! bits[random_bit];
        EV << bits.to_string() << endl;
        // convert the binary representation to char after modification
        s[random_var] = (char)bits.to_ulong();
        std::stringstream ss;
        ss << s;
        // update the payload to the modified one
        msg->setMessage_payload(ss.str().c_str());
        // print payload after modification to assure modification happens
        EV << " message after modification " << s << endl;
    }
    // calculate the probability to duplicate the message
    int prob_duplicate  = uniform(0,1)*10;
    // in case of message duplication
    if (prob_duplicate > par("prob_duplication").doubleValue())
    {
        EV << "message duplicated" <<endl;
        // check if the message also delayed return 2 else return 3
        if (choice == 2)
        {
            return 2;
        }
        else
        {
            return 3;
        }
    }
    // if the message not duplicated but delayed return 1
    if (choice == 2)
    {
        return 1;
    }
    // if none return 4
    return 4;
}
/**
 *handle any message receiving logic
 @param msg : message received from the connected gates.
*/
void Hub::handleMessage(cMessage *msg)
{
    // self messages to end current session and start new one or print system statistics
    if (msg->isSelfMessage())
    {
        // in case of session starting
        if (msg->getKind() == 1)
        {
            // allocate new session for 2 random nodes to communicate with each other
            startSession();
        
        }
        // in case of printing system statistics
        else
        {
            // when receiving a self message of kind 2 print statistics (collective)
            this->printStats(true);
            scheduleAt(simTime() + par("stats_time"), this->print_stats);
        }
     // receiving a message from node
    }
    else
    {
        // normal message process it normally
        try
        {
            // cast the message
            Imessage_Base * rmsg = check_and_cast<Imessage_Base *> (msg);
            // navigate the message to its corresponding receiver
            parseMessage(rmsg);
        }
        // end session message
        catch(...)
        {
            EV << "session ends early and starting a new one now" <<endl;
            //cancel the end session event and start a new session now
            cancelAndDelete(msg);
            cancelEvent(this->start_session);
            startSession();
        }
    }
}
/**
 * navigate the message from its sender to its corresponding receivers adding channel noise effect.
 @param msg : message received from the connected gates.
*/
void Hub::parseMessage(Imessage_Base * msg)
{
    // get the message sender
    int msg_sender = msg->getSenderModule()->getIndex();
    // logs for debugging
    EV << " the message sender id = "<< msg_sender << endl;
    EV << " the hub sender id = " << sender << endl;
    EV << " the hub receiver id = " << receiver << endl;
    // if a message was received from previous session ignore it
    if (msg_sender != this->sender && msg_sender != this->receiver)
    {
        cancelAndDelete(msg);
        return;
    }
    // get the message receiver
    int msg_receiver;
    if (msg_sender == this->sender)
    {
        msg_receiver = this->receiver;
    }
    else
    {
        msg_receiver = this->sender;
    }
    // apply the transmission noise
    int noise = applyNoise(msg);
    // calculate the amount of delay
    int delay = exponential( par("mean_delay").doubleValue());
    EV << " delay time = " << delay <<endl;
    std::string msg_payload = msg->getMessage_payload();
    //select how to send the message based on the noise effect
    switch(noise)
    {
        // lose the message
        case 0:
        {
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), true, false);
            cancelAndDelete(msg);
            return;
        }
        // delayed , delayed and modified
        case 1:
        {
            EV <<  " message delayed"  <<endl;
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, false);
            sendDelayed(msg, delay, "outs", msg_receiver);
            break;
        }
        // delayed duplicate , delayed duplicate and modified
        case 2:
        {
            EV << "message delayed " <<endl;
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, true);
            Imessage_Base * msg2 = msg->dup();
            sendDelayed(msg, delay, "outs", msg_receiver);
            sendDelayed(msg2,delay, "outs", msg_receiver);
            break;
        }
        case 3:
        {
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, true);
            Imessage_Base * msg2 = msg->dup();
            send(msg, "outs", msg_receiver);
            send(msg2, "outs", msg_receiver);
            break;
        }
        // send the message normally or modified only
        default :
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, false);
            send(msg, "outs", msg_receiver );
            break;
    }
}

// ------------------------------------------- statistics utilities --------------------------------------------

/**
 * initialize statistics variables for all n nodes
*/
void Hub::initializeStats()
{
    // loop over all n nodes and insert and empty vector in statistics variables
    for (int i = 0; i < this->n; i++)
    {
        this->generated_frames.push_back(std::vector<int>());
        this->frame_sizes.push_back(std::vector<int>());
        this->is_dropped.push_back(std::vector<int>());
        this->is_duplicated.push_back(std::vector<int>());
        this->retransmitted_frames.push_back(std::vector<int>());
    }
}
/**
 * update node statistics
 @param node_idx : node index
 @param seq_num : sequence number of sent frame
 @param frame_size : size of sent frame
 @param is_dropped : whether the frame is dropped
 @param is_duplicated : whether the frame is duplicated
*/
void Hub::updateStats(int node_idx, int seq_num, int frame_size, bool is_dropped, bool is_duplicated)
{
    // check whether the sequence number is already in the node generated frames
    auto it = std::find(this->generated_frames[node_idx].begin(), this->generated_frames[node_idx].end(), seq_num);
    if(it != this->generated_frames[node_idx].end())
    {
        // if sequence number is already in the node generated frames -> retransmitted
        int index = it - this->generated_frames[node_idx].begin();
        // increase retransmission times
        this->retransmitted_frames[node_idx][index] ++;
        // increase drop times (if dropped)
        if (is_dropped)
        {
            this->is_dropped[node_idx][index] ++;
        }
        // increase duplication times (if duplicated)
        if (is_duplicated)
        {
            this->is_duplicated[node_idx][index] ++;
        }
    }
    else
    {
        // if sequence number is not in the node generated frames -> generated
        // add new generated frame
        this->generated_frames[node_idx].push_back(seq_num);
        this->frame_sizes[node_idx].push_back(frame_size);
        // set retransmission times to 0
        this->retransmitted_frames[node_idx].push_back(0);
        // set drop times to 1 (if dropped)
        if (is_dropped)
        {
            this->is_dropped[node_idx].push_back(1);
        }
        else
        {
            this->is_dropped[node_idx].push_back(0);
        }
        // set duplication times to 1 (if duplicated)
        if (is_duplicated)
        {
            this->is_duplicated[node_idx].push_back(1);
        }
        else
        {
            this->is_duplicated[node_idx].push_back(0);
        }
    }
}
/**
 * print statistics (separate or collective)
 @param collective : whether to print collective or separate logs
*/
void Hub::printStats(bool collective)
{
    // check whether statistics print is collective or separater
    if (collective)
    {
        // count all generated frames
        int total_generated = 0;
        for (int i = 0; i < this->n; i++)
        {
            total_generated += this->generated_frames[i].size();
        }
        // count all dropped frames
        int total_dropped = 0;
        for (int i = 0; i < this->n; i++)
        {
            total_dropped += std::accumulate(this->is_dropped[i].begin(), this->is_dropped[i].end(), 0);
        }
        // count all retransmitted frames
        int total_retransmitted = 0;
        for (int i = 0; i < this->n; i++)
        {
            total_retransmitted += std::accumulate(this->retransmitted_frames[i].begin(), this->retransmitted_frames[i].end(), 0);
        }
        // count useful data using generated frames sizes of messages payload
        float useful_data = 0;
        // count all transmitted data using all generated, retransmitted and duplicated frames with headers
        float all_data = 0;
        for (int i = 0; i < this->n; i++)
        {
            for (int j = 0; j < this->generated_frames[i].size(); j++)
            {
                useful_data += this->frame_sizes[i][j];
                all_data += (this->retransmitted_frames[i][j] + this->is_duplicated[i][j] + 1) * (4*3 + this->frame_sizes[i][j]);
            }
        }
        // get efficiency (useful data / all data)
        float efficieny = useful_data/all_data;
        // print collective logs
        EV << "####################### Collective Statistics #######################" << endl;
        EV << "Total number of generated frames = " << total_generated << endl;
        EV << "Total number of dropped frames = " << total_dropped << endl;
        EV << "Total number of retransmitted frames = " << total_retransmitted << endl;
        EV << "Percentage of useful transmitted data = " << efficieny*100 << "%" << endl;
    }
    else
    {
        // loop over each node
        for (int i = 0; i < this->n; i++)
        {
            // count all generated frames
            int total_generated = this->generated_frames[i].size();
            // count all dropped frames
            int total_dropped = std::accumulate(this->is_dropped[i].begin(), this->is_dropped[i].end(), 0);
            // count all retransmitted frames
            int total_retransmitted = std::accumulate(this->retransmitted_frames[i].begin(), this->retransmitted_frames[i].end(), 0);
            // count useful data using generated frames sizes of messages payload
            float useful_data = 0;
            // count all transmitted data using all generated, retransmitted and duplicated frames with headers
            float all_data = 0;
            for (int j = 0; j < this->generated_frames[i].size(); j++)
            {
                useful_data += this->frame_sizes[i][j];
                all_data += (this->retransmitted_frames[i][j] + this->is_duplicated[i][j] + 1) * (4*3 + this->frame_sizes[i][j]);
            }
            // get efficiency (useful data / all data)
            float efficieny = useful_data/all_data;
            // print separate node logs
            EV << "####################### Node [" << i << "] Statistics #######################" << endl;
            EV << "Total number of generated frames = " << total_generated << endl;
            EV << "Total number of dropped frames = " << total_dropped << endl;
            EV << "Total number of retransmitted frames = " << total_retransmitted << endl;
            EV << "Percentage of useful transmitted data = " << efficieny*100 << "%" << endl;
        }
    }
}
