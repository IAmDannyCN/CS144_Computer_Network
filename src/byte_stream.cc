#include "byte_stream.hh"

using namespace std;

#ifdef BYTE_DEBUG
string _Out( string s )
{
  string ans = "";
  for ( char c : s ) {
    ans += std::to_string( (int)c ) + " ";
  }
  return ans;
}
string _Out( string_view s )
{
  string ans = "";
  for ( char c : s ) {
    ans += std::to_string( (int)c ) + " ";
  }
  return ans;
}
#endif

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), buffer_(), buffer_view_(), closed_( false ), curlen_( 0 ), pushcnt_( 0 ), popcnt_( 0 )
{
#ifdef BYTE_DEBUG
  cout << "capacity: " << capacity << endl;
#endif
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( closed_ ) {
    set_error();
    return;
  }

#ifdef BYTE_DEBUG
  cout << "PUSH:" << _Out( data ) << endl;
#endif

  uint64_t size_left = capacity_ - curlen_;
  uint64_t size_use = min( size_left, data.size() );

  if ( size_use == 0 ) {
    return;
  }

  if ( size_use == size_left ) {
    data = data.substr( 0, size_use );
  }

  buffer_.push_back( std::move( data ) );
  buffer_view_.push_back( std::string_view( buffer_.back().c_str(), buffer_.back().size() ) );

  curlen_ += size_use;
  pushcnt_ += size_use;
  return;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - curlen_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushcnt_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && curlen_ == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return popcnt_;
}

string_view Reader::peek() const
{
  // Your code here.
  if ( buffer_view_.empty() ) {
    // set_error();
    return {};
  }
#ifdef BYTE_DEBUG
  cout << ">>>>";
  for ( long unsigned int i = 0; i < buffer_.size(); i++ )
    cout << "| " << _Out( buffer_[i] );
  cout << endl;
  cout << "++++";
  for ( long unsigned int i = 0; i < buffer_view_.size(); i++ )
    cout << "| " << _Out( buffer_view_[i] );
  cout << endl;
#endif
  return buffer_view_.front();
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len > curlen_ ) {
    set_error();
    return;
  }
  if ( len == 0 ) {
    return;
  }

#ifdef BYTE_DEBUG
  cout << "POP:" << len << endl;
#endif

  uint64_t left = len;
  while ( left ) {
    if ( left >= buffer_view_.front().size() ) {
      left -= buffer_view_.front().size();
      buffer_.pop_front();
      buffer_view_.pop_front();
      continue;
    }
    buffer_view_.front().remove_prefix( left );
    left = 0;
  }

  curlen_ -= len;
  popcnt_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return curlen_;
}
