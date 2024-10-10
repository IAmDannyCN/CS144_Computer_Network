#include "tcp_receiver.hh"

using namespace std;

// #define TCPRECEIVER_DEBUG

#ifdef TCPRECEIVER_DEBUG
string _Out( string s )
{
  string ans = "";
  for ( char c : s ) {
    ans += std::to_string( (int)c ) + "/";
  }
  return ans;
}
#endif

void TCPReceiver::receive( TCPSenderMessage message )
{
#ifdef TCPRECEIVER_DEBUG
    cout << endl << "[Receive]" << endl;
    cout << "receive message: SYN=" << message.SYN <<", FIN=" << message.FIN << ", RST=" << message.RST << ", seqno=" << message.seqno.get_Raw_Value_() <<  " , payload with length " << message.payload.size() << ": <" << _Out(message.payload) << ">" << endl;
#endif
    // Your code here.
    if(message.RST == true) {
        RST_set = true;
        reassembler_.set_writer_error_();
    }
    if(message.SYN == false && SYN_set == false) {
        return ;
    }
    if(message.SYN == true) {
        SYN_set = true;
        SYN = message.seqno;
        message.seqno = message.seqno + (uint32_t)1;
    }
    if(message.FIN == true) {
        FIN_set = true;
    }
    
    // if(message.payload.size() > 0) {
        uint64_t first_index = message.seqno.unwrap(SYN, reassembler_.get_Base_Position_()) - 1;
    #ifdef TCPRECEIVER_DEBUG
        cout << ">>>" << "Insert: first_index = " << first_index << ", payload = " << message.payload << ", finished = " << message.FIN << endl;
    #endif
        reassembler_.insert(first_index, message.payload, message.FIN);
    // }
}

TCPReceiverMessage TCPReceiver::send() const
{
    // Your code here.
    // return {};
#ifdef TCPRECEIVER_DEBUG
    cout << endl << "[Send]" << endl;
#endif
    TCPReceiverMessage message;
    if(SYN_set) {
    #ifdef TCPRECEIVER_DEBUG
        cout << "Base_Position = " << reassembler_.get_Base_Position_() << endl;
    #endif
        message.ackno = Wrap32::wrap(reassembler_.get_Base_Position_() + SYN_set + reassembler_.writer().is_closed(), SYN);
    }
#ifdef TCPRECEIVER_DEBUG
    cout << "BS: available = " << reassembler_.writer().available_capacity() << ", RA: pending = " << reassembler_.bytes_pending() << endl;
#endif
    message.window_size = (uint16_t)min(reassembler_.writer().available_capacity(), (uint64_t)UINT16_MAX);
    message.RST = RST_set || reassembler_.writer().has_error();
    return message;
}
