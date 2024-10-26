#include "reassembler.hh"

using namespace std;

// #define REASSEMBLE_DEBUG

#ifdef REASSEMBLE_DEBUG
string _Out( string s )
{
  string ans = "";
  for ( char c : s ) {
    ans += std::to_string( (int)c ) + "/";
  }
  return ans;
}
#endif

void Reassembler::insert( uint64_t first_index, std::string data, bool is_last_substring )
{
#ifdef REASSEMBLE_DEBUG
  cout << "---------" << endl;
  cout << "Accepted: <" << _Out( data ) << "> at position " << first_index << ", is_last = " << is_last_substring
       << endl;
#endif
  // Your code here.
  // (void)first_index;
  // (void)data;
  // (void)is_last_substring;
  uint64_t buffer_begin = base_position_;
  uint64_t buffer_end = base_position_ + output_.writer().available_capacity();
  bool is_data_complete = true;

  if ( first_index >= buffer_end || first_index + (uint64_t)data.size() < buffer_begin ) {
#ifdef REASSEMBLE_DEBUG
    cout << "Finished Doing Nothing. 1" << endl;
    cout << "---------" << endl;
#endif
    return;
  }
  if ( first_index < buffer_begin ) {
    data = data.substr( buffer_begin - first_index );
    first_index = buffer_begin;
    if ( first_index >= buffer_end || first_index + (uint64_t)data.size() < buffer_begin ) {
#ifdef REASSEMBLE_DEBUG
      cout << "Finished Doing Nothing. 1.5" << endl;
      cout << "---------" << endl;
#endif
      return;
    }
  }
  if ( first_index + (uint64_t)data.size() >= buffer_end ) {
    data.resize( buffer_end - first_index );
    is_data_complete = false;
  }
  // Now "data" fit in the buffer area
#ifdef REASSEMBLE_DEBUG
  cout << "DATA_0: <" << _Out( data ) << ">" << endl;
#endif

  auto insertpos = list_.end();
  bool insertpos_default = true;
  for ( auto it = list_.begin(); it != list_.end(); ) {

    if ( it->idx <= first_index && ( it->idx + (uint64_t)it->len ) >= ( first_index + (uint64_t)data.size() ) ) {
      // data is fully contained by *it
#ifdef REASSEMBLE_DEBUG
      cout << "Finished Doing Nothing. 2" << endl;
      cout << "---------" << endl;
#endif
      return;
    } else if ( it->idx >= first_index
                && ( it->idx + (uint64_t)it->len ) <= ( first_index + (uint64_t)data.size() ) ) {
      //: *it is fully contained by data
#ifdef REASSEMBLE_DEBUG
      cout << "Branch A." << endl;
#endif
      bytes_pending_ -= it->len;
      it = list_.erase( it );
#ifdef REASSEMBLE_DEBUG
      for ( auto& node : list_ ) {
        std::cout << "0LIST [" << node.idx << "] (" << node.len << ") " << _Out( node.content ) << endl;
      }
#endif
      continue;
    } else {
      if ( it->idx >= first_index && insertpos_default ) {
        insertpos = it; // DANGEROUS!!!! it may be deleted!!!!!
        insertpos_default = false;
      }
      if ( it->idx < first_index && ( it->idx + (uint64_t)it->len ) < ( first_index + (uint64_t)data.size() )
           && ( it->idx + (uint64_t)it->len ) > first_index ) {
        // data    XXXX
        //: *it  XXXX
#ifdef REASSEMBLE_DEBUG
        cout << "Branch B." << endl;
#endif
        it->content.resize( first_index - it->idx );
        bytes_pending_ -= it->len - it->content.size();
        it->len = it->content.size();
      } else if ( it->idx > first_index && ( it->idx + (uint64_t)it->len ) > ( first_index + (uint64_t)data.size() )
                  && ( first_index + (uint64_t)data.size() ) > it->idx ) {
        // data  XXXX
        //: *it    XXXX
#ifdef REASSEMBLE_DEBUG
        cout << "Branch C." << endl;
#endif
        data.resize( it->idx - first_index );
        is_data_complete = false;
      }
    }
    it++;
  }
#ifdef REASSEMBLE_DEBUG
  std::string data_bak = data;
  for ( auto& node : list_ ) {
    std::cout << "1LIST [" << node.idx << "] (" << node.len << ") " << _Out( node.content ) << endl;
  }
#endif
  uint64_t curdata_size = data.size();
  bytes_pending_ += curdata_size;
  // list_.insert(insertpos, Node(data.size(), first_index, std::move(data), is_data_complete &&
  // is_last_substring));
  if ( insertpos_default ) {
    list_.push_back( Node( curdata_size, first_index, std::move( data ), is_data_complete && is_last_substring ) );
  } else {
    list_.insert( insertpos,
                  Node( curdata_size, first_index, std::move( data ), is_data_complete && is_last_substring ) );
  }

#ifdef REASSEMBLE_DEBUG
  cout << "Pending: " << bytes_pending_ << endl;
  cout << "DATA: <" << _Out( data_bak ) << ">" << endl;
  for ( auto& node : list_ ) {
    cout << ">>>";
    cout << "2LIST [" << node.idx << "] (" << node.len << ") " << _Out( node.content ) << endl;
  }
#endif

  uint64_t expected = base_position_;
  int deletecnt = 0;
  for ( auto& node : list_ ) {
    if ( node.idx == expected ) {
      expected += node.len;
      bytes_pending_ -= node.len;
      base_position_ += node.len;
#ifdef REASSEMBLE_DEBUG
      cout << "Pushed: <" << _Out( node.content ) << ">, is_final = " << node.is_final
           << ", now base_position_ = " << base_position_ << endl;
#endif
      output_.writer().push( std::move( node.content ) );
      // output_.writer().push(node.content);
      if ( node.is_final ) {
        output_.writer().close();
      }
      deletecnt++;
    }
  }
  while ( deletecnt-- ) {
    list_.pop_front();
  }
#ifdef REASSEMBLE_DEBUG
  for ( auto& node : list_ ) {
    std::cout << "3LIST [" << node.idx << "] (" << node.len << ") " << _Out( node.content ) << endl;
  }
  cout << "Finished. 3" << endl;
  cout << "---------" << endl;
#endif
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
