#include "FragSpectrumScanDatabase.h"

struct underflow_info
{
    xml_schema::buffer* buf;
    size_t pos;
};

extern "C" int
overflow (void* user_data, char* buf, int n);

extern "C" int
underflow (void* user_data, char* buf, int n);



extern "C"
typedef  void (*xdrrec_create_p) (
    XDR*,
    unsigned int write_size,
    unsigned int read_size,
    void* user_data,
    int (*read) (void* user_data, char* buf, int n),
    int (*write) (void* user_data, char* buf, int n));



FragSpectrumScanDatabase::FragSpectrumScanDatabase(string id_par) :
    bdb(0), scan2rt(0) {
  xdrrec_create_p xdrrec_create_ = reinterpret_cast<xdrrec_create_p> (::xdrrec_create);
  xdrrec_create_ (&xdr, 0, 0, reinterpret_cast<char*> (&buf), 0, &overflow);
  xdr.x_op = XDR_ENCODE;
  std::auto_ptr< xml_schema::ostream<XDR> > tmpPtr(new xml_schema::ostream<XDR>(xdr)) ;
  assert(tmpPtr.get());
  oxdrp=tmpPtr;
  if(id_par.empty()) id = "no_id"; else id = id_par;
}


void FragSpectrumScanDatabase::savePsm( unsigned int scanNr,
    std::auto_ptr< percolatorInNs::peptideSpectrumMatch > psm_p ) {

  std::auto_ptr< ::percolatorInNs::fragSpectrumScan>  fss = getFSS( scanNr );
  // if FragSpectrumScan does not yet exist, create it
  if ( ! fss.get() ) {
    std::auto_ptr< ::percolatorInNs::fragSpectrumScan>
    fs_p( new ::percolatorInNs::fragSpectrumScan(scanNr));
    fss = fs_p;
  }
  // add the psm to the FragSpectrumScan
  fss->peptideSpectrumMatch().push_back(psm_p);
  putFSS( *fss );
  return;
}

//   The function "tcbdbopen" in Tokyo Cabinet does not have O_EXCL as is
//   possible in the unix system call open (see "man 2 open"). This may be a
//   security issue if the filename to the Tokyo cabinet database is in a
//   directory that other users have write access to. They could add a symbolic
//   link pointing somewhere else. It would be better if Tokyo Cabinet would
//   fail if the database existed in our case when we use a temporary file.
bool FragSpectrumScanDatabase::init(std::string fileName) {

  #if defined __LEVELDB__
    options.create_if_missing = true;
    options.error_if_exists = true;
    leveldb::Status status = leveldb::DB::Open(options, fileName.c_str(), &bdb);
    if (!status.ok()){ 
      std::cerr << status.ToString() << endl;
      exit(EXIT_FAILURE);
    }
    bool ret = status.ok();
  #else
    bdb = tcbdbnew();
    assert(bdb);
    bool ret =  tcbdbsetcmpfunc(bdb, tccmpint32, NULL);
    assert(ret);
    if(!tcbdbopen(bdb, fileName.c_str(), BDBOWRITER | BDBOTRUNC | BDBOREADER | BDBOCREAT )){
      int errorcode = tcbdbecode(bdb);
      fprintf(stderr, "open error: %s\n", tcbdberrmsg(errorcode));
      exit(EXIT_FAILURE);
    }
  #endif
  ret = unlink( fileName.c_str() );
  assert(! ret);
  return ret;
}

void FragSpectrumScanDatabase::terminte(){
  #if defined __LEVELDB__
    delete bdb;
  #else
    tcbdbdel(bdb);   
  #endif
}

bool FragSpectrumScanDatabase::initRTime(map<int, vector<double> >* scan2rt_par) {
  // add pointer to retention times table in sqt2pin (if any)
  scan2rt=scan2rt_par;
}

std::auto_ptr< ::percolatorInNs::fragSpectrumScan> FragSpectrumScanDatabase::deserializeFSSfromBinary( char * value, int valueSize ) {
  xml_schema::buffer buf2;
  buf2.capacity(valueSize);
  memcpy(buf2.data(), value, valueSize);
  buf2.size(valueSize);
  underflow_info ui;
  ui.buf = &buf2;
  ui.pos = 0;
  XDR xdr2;
  xdrrec_create_p xdrrec_create_ = reinterpret_cast<xdrrec_create_p> (::xdrrec_create);
  xdrrec_create_ (&xdr2, 0, 0, reinterpret_cast<char*> (&ui), &underflow, 0);
  xdr2.x_op = XDR_DECODE;
  xml_schema::istream<XDR> ixdr(xdr2);
  xdrrec_skiprecord(&xdr2);
  std::auto_ptr< percolatorInNs::fragSpectrumScan> fss (new percolatorInNs::fragSpectrumScan(ixdr));
  //TOFIX it gives too many arguments in MINGW
  //xdr_destroy (&xdr2);
  if((&xdr2)->x_ops->x_destroy)			
  {
#if defined __MINGW__ or defined __WIN32__
    (*(&xdr2)->x_ops->x_destroy);
#else
    (*(&xdr2)->x_ops->x_destroy)(&xdr2);
#endif
  }
  return fss;
}

std::auto_ptr< ::percolatorInNs::fragSpectrumScan> FragSpectrumScanDatabase::getFSS( unsigned int scanNr ) {
  #if defined __LEVELDB__
    assert(bdb);
    int valueSize = 0;
    std::string value;
    std::stringstream out;
    out << scanNr;
    leveldb::Slice s1 = out.str(); //be careful with the scope of the slice
    leveldb::Status s = bdb->Get(leveldb::ReadOptions(), s1, &value);
    if(!s.ok()){
      return std::auto_ptr< ::percolatorInNs::fragSpectrumScan> (NULL);
    }
    char *retvalue=new char[value.size()+1];
    retvalue[value.size()]=0;
    memcpy(retvalue,value.c_str(),value.size());
    std::auto_ptr< ::percolatorInNs::fragSpectrumScan> ret(deserializeFSSfromBinary(retvalue, (int)sizeof(retvalue) )); 
  #else
    assert(bdb);
    int valueSize = 0;
    char * value = ( char * ) tcbdbget(bdb, ( const char* ) &scanNr, sizeof( scanNr ), &valueSize);
    if(!value) {
      return std::auto_ptr< ::percolatorInNs::fragSpectrumScan> (NULL);
    }
    std::auto_ptr< ::percolatorInNs::fragSpectrumScan> ret(deserializeFSSfromBinary(value,valueSize));  
    free(value);
  #endif
  return ret;
}

void FragSpectrumScanDatabase::print(serializer & ser) {
  #if defined __LEVELDB__
    assert(bdb);
    leveldb::Iterator* it = bdb->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      char *retvalue=new char[it->value().ToString().size()+1];
      retvalue[it->value().ToString().size()]=0;
      memcpy(retvalue,it->value().ToString().c_str(),it->value().ToString().size());
      std::auto_ptr< ::percolatorInNs::fragSpectrumScan> fss(deserializeFSSfromBinary(retvalue,(int)sizeof(retvalue)));
      ser.next ( PERCOLATOR_IN_NAMESPACE, "fragSpectrumScan", *fss);
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;
  #else
    BDBCUR *cursor;
    char *key;
    assert(bdb);
    cursor = tcbdbcurnew(bdb);
    assert(cursor);
    tcbdbcurfirst(cursor);
    // using tcbdbcurkey3 is probably faster
    int keySize;
    int valueSize;
    while (( key = static_cast< char * > ( tcbdbcurkey(cursor,&keySize)) ) != 0 ) {
      char * value = static_cast< char * > ( tcbdbcurval(cursor,&valueSize));
      if(value){
	std::auto_ptr< ::percolatorInNs::fragSpectrumScan> fss(deserializeFSSfromBinary(value,valueSize));
	ser.next ( PERCOLATOR_IN_NAMESPACE, "fragSpectrumScan", *fss);
	free(value);
      }
      free(key);
      tcbdbcurnext(cursor);
    }
    tcbdbcurdel(cursor);
  #endif
}

void FragSpectrumScanDatabase::putFSS( ::percolatorInNs::fragSpectrumScan & fss ) {
  #if defined __LEVELDB__
    assert(bdb);
    *oxdrp << fss;
    xdrrec_endofrecord (&xdr, true);
    leveldb::WriteOptions write_options;
    //careful in windows
    write_options.sync = true;
    std::stringstream out;
    out << (unsigned int)fss.scanNumber();
    leveldb::Slice s1 = out.str();
    leveldb::Status status = bdb->Put(write_options,s1,buf.data());
    if(!status.ok()){
      std::cerr << status.ToString() << endl;
      exit(EXIT_FAILURE);
    }
  #else
    assert(bdb);
    *oxdrp << fss;
    xdrrec_endofrecord (&xdr, true);
    ::percolatorInNs::fragSpectrumScan::scanNumber_type key = fss.scanNumber();
    size_t keySize = sizeof(key);
    size_t valueSize(buf.size ());
    if(!tcbdbput(bdb, ( const char * ) &key, keySize, buf.data (), buf.size () ))
    {
      int  errorcode = tcbdbecode(bdb);
      fprintf(stderr, "put error: %s\n", tcbdberrmsg(errorcode));
      exit(EXIT_FAILURE);
    }  
  #endif
  buf.size(0);
  return;
}

extern "C" int
overflow (void* p, char* buf, int n_)
{
  xml_schema::buffer* dst (reinterpret_cast<xml_schema::buffer*> (p));
  size_t n (static_cast<size_t> (n_));
  size_t size (dst->size ());
  size_t capacity (dst->capacity ());
  if (size + n > capacity && size + n < capacity * 2)
    dst->capacity (capacity * 2);
  dst->size (size + n);
  memcpy (dst->data () + size, buf, n);
  return n;
}

extern "C" int
underflow (void* p, char* buf, int n_)
{
  underflow_info* ui (reinterpret_cast<underflow_info*> (p));
  size_t n (static_cast<size_t> (n_));
  size_t size (ui->buf->size () - ui->pos);
  n = size > n ? n : size;
  memcpy (buf, ui->buf->data () + ui->pos, n);
  ui->pos += n;
  return n;
}
