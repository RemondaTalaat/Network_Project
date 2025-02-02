//
// Generated file, do not edit! Created by nedtool 5.6 from Imessage.msg.
//

#ifndef __IMESSAGE_M_H
#define __IMESSAGE_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>
#include <string>
// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>Imessage.msg:20</tt> by nedtool.
 * <pre>
 * packet Imessage
 * {
 *     \@customize(true);                   // see the generated C++ header for more info
 *     int sequence_number;                // message sequence number
 *     int pad_length;                     // the padding length ( used in hamming )
 *     int acknowledge;                    // the acknowledge of the last received frame
 *     string message_payload;             // message content framed by char count
 * }
 * </pre>
 *
 * Imessage_Base is only useful if it gets subclassed, and Imessage is derived from it.
 * The minimum code to be written for Imessage is the following:
 *
 * <pre>
 * class Imessage : public Imessage_Base
 * {
 *   private:
 *     void copy(const Imessage& other) { ... }

 *   public:
 *     Imessage(const char *name=nullptr, short kind=0) : Imessage_Base(name,kind) {}
 *     Imessage(const Imessage& other) : Imessage_Base(other) {copy(other);}
 *     Imessage& operator=(const Imessage& other) {if (this==&other) return *this; Imessage_Base::operator=(other); copy(other); return *this;}
 *     virtual Imessage *dup() const override {return new Imessage(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from Imessage_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(Imessage)
 * </pre>
 */
class Imessage_Base : public ::omnetpp::cPacket
{
  protected:
    int sequence_number;
    int pad_length;
    int acknowledge;
    std::string message_payload;

  private:
    void copy(const Imessage_Base& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Imessage_Base&);
    // make constructors protected to avoid instantiation

    Imessage_Base(const Imessage_Base& other);
    // make assignment operator protected to force the user override it
    Imessage_Base& operator=(const Imessage_Base& other);

  public:
    Imessage_Base(const char *name=nullptr, short kind=0);
    virtual ~Imessage_Base();
    virtual Imessage_Base *dup() const override {return new Imessage_Base (*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getSequence_number() const;
    virtual void setSequence_number(int sequence_number);
    virtual int getPad_length() const;
    virtual void setPad_length(int pad_length);
    virtual int getAcknowledge() const;
    virtual void setAcknowledge(int acknowledge);
    virtual std::string getMessage_payload() const;
    virtual void setMessage_payload(std::string message_payload);
};


#endif // ifndef __IMESSAGE_M_H

