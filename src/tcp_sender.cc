#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "wrapping_integers.hh"

using namespace std;

// #define TCPSENDER_DEBUG

#ifdef TCPSENDER_DEBUG
string _Out( string s )
{
  string ans = "";
  for ( char c : s ) {
    ans += std::to_string( (int)c ) + "/";
  }
  return ans;
}
#endif

uint64_t TCPSender::sequence_numbers_in_flight() const
{
    // Your code here.
    return n_outstanding_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
    // Your code here.
    return n_retransmission_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
#ifdef TCPSENDER_DEBUG
    cout << endl << "-------------------------------" << endl;
    cout << "[PUSH CALLED]" << endl;
#endif

    // Your code here.
    // (void)transmit;
    int modified_window_size_ = window_size_ == 0 ? 1 : window_size_;
    while(modified_window_size_ > n_outstanding_) {
    
    #ifdef TCPSENDER_DEBUG
        cout << "--------" << endl;
        cout << "window_size = " << window_size_  << "(" << modified_window_size_ << ")" << ", n_outstanding = " << n_outstanding_ << endl;
    #endif

        // if(reader().bytes_buffered() == 0) {
        //     return ;
        // }
        if(FIN_sent_) {
            return ;
        }

        TCPSenderMessage msg = make_empty_message();
        if(!SYN_sent_) {
            SYN_sent_ = true;
            msg.SYN = true;
        }
        // uint64_t len = min(min(reader().bytes_buffered(), modified_window_size_ - n_outstanding_ - msg.sequence_length()), TCPConfig::MAX_PAYLOAD_SIZE);
        uint64_t MAX_len = min(modified_window_size_ - n_outstanding_ - msg.sequence_length(), TCPConfig::MAX_PAYLOAD_SIZE);
        uint64_t MAX_len_for_FIN = modified_window_size_ - n_outstanding_ - msg.sequence_length();
        uint64_t pending_len = 0;

        while(input_.reader().bytes_buffered()) {
            string_view content = reader().peek();
            uint64_t cur_len = min(MAX_len - pending_len, content.size());
            msg.payload += content.substr(0, cur_len);
            input_.reader().pop(cur_len);
            pending_len += cur_len;
            if(pending_len == MAX_len) {
                break;
            }
        }
        if(pending_len < MAX_len_for_FIN && !FIN_sent_ && reader().is_finished()) {
            msg.FIN = true;
            FIN_sent_ = true;
        }

        if(msg.sequence_length() == 0) {
            return ;
        }

    #ifdef TCPSENDER_DEBUG
        cout << "msg seqno=" << msg.seqno.get_Raw_Value_() << ", sequence_length=" << msg.sequence_length() << endl; 
        cout << "    " << "SYN=" << msg.SYN << ", FIN=" << msg.FIN << ", RST=" << msg.RST << "; " << _Out(msg.payload) << endl;
    #endif

        transmit(msg);

        next_seqno_ += msg.sequence_length();
        n_outstanding_ += msg.sequence_length();

        if(!ticker_.is_running()) {
            ticker_.start();
        }
        
        outstanding_messages_.push_back(msg);
        if(window_size_ == 0) {
            no_RTO_backoff_.insert(msg.seqno.get_Raw_Value_());
        }
        
    }

#ifdef TCPSENDER_DEBUG
    cout << "<Outstanding Messages> " << outstanding_messages_.size() << " in total." << endl;
    for(auto message: outstanding_messages_) {
        cout << "Message: SYN=" << message.SYN <<" ,FIN="<< message.FIN <<" ,RST="<< message.RST <<" ,seqno="<< message.seqno.get_Raw_Value_() <<"; "<< _Out(message.payload) << endl;
    }
    cout << "===============================" << endl;
#endif
}

TCPSenderMessage TCPSender::make_empty_message() const
{
    // Your code here.
    TCPSenderMessage msg;
    msg.SYN = false;
    msg.FIN = false;
    msg.RST = input_.has_error();
    msg.payload = "";
    msg.seqno = Wrap32::wrap(next_seqno_, Wrap32(isn_));
    return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
#ifdef TCPSENDER_DEBUG
    cout << endl << "-------------------------------" << endl;
    cout << "[RECEIVE CALLED]" << endl;
    cout << "RST = " << msg.RST << ", ackno = " << msg.ackno->get_Raw_Value_() << endl;
    cout << "===============================" << endl;
#endif

    // Your code here.
    // (void)msg;
    if(msg.RST) {
        input_.set_error();
        return ;
    }

    window_size_ = msg.window_size;

    uint64_t received_seqno = msg.ackno->unwrap(isn_, next_seqno_);
    if(received_seqno > next_seqno_) {
        return ;
    }

    bool hav_ack = false;
    while(!outstanding_messages_.empty()) {
        uint64_t cur_length = outstanding_messages_.front().sequence_length();
        if(acked_seqno_ + cur_length > received_seqno) {
            break;
        }
        acked_seqno_ += cur_length;
        n_outstanding_ -= cur_length;
        if(no_RTO_backoff_.find(outstanding_messages_.front().seqno.get_Raw_Value_()) != no_RTO_backoff_.end()) {
            no_RTO_backoff_.erase(outstanding_messages_.front().seqno.get_Raw_Value_());
        }
        outstanding_messages_.pop_front();
        hav_ack = true;
    }

    if(hav_ack) {
        n_retransmission_ = 0;
        ticker_.set_RTO_ms(initial_RTO_ms_);
        if(outstanding_messages_.empty()) {
            ticker_.stop();
        } else {
            ticker_.start();
        }
    }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
    // Your code here.
    // (void)ms_since_last_tick;
    // (void)transmit;
    // (void)initial_RTO_ms_;

#ifdef TCPSENDER_DEBUG
    cout << "[TICK] " << ms_since_last_tick << endl;
#endif

    ticker_.tick(ms_since_last_tick);

#ifdef TCPSENDER_DEBUG
    cout << "is_expired() = " << ticker_.is_expired() << endl;
#endif

    if(!ticker_.is_expired()) {
        return ;
    }
    if(n_outstanding_ == 0) {
        return ;
    }

#ifdef TCPSENDER_DEBUG
    cout << "ReSend: " << outstanding_messages_.front().seqno.get_Raw_Value_() << endl;
#endif

    transmit(outstanding_messages_.front());
    n_retransmission_ += 1;
    
    if(no_RTO_backoff_.find(outstanding_messages_.front().seqno.get_Raw_Value_()) == no_RTO_backoff_.end()) {
        ticker_.exponential_backoff();
    }
    ticker_.reset();
}
