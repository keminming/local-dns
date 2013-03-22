/*
	KE Wang
	822002009
*/
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <Winsock2.h>
#include <utility>	
#include <fstream>
#include "socket.h"
#include <Windows.h>
#include <queue>
#include <Iphlpapi.h>
using namespace std;

/* flags */
#define DNS_QUERY (0 << 15) /* 0 = query; 1 = response */
#define DNS_RESPONSE (1 << 15)
#define DNS_STDQUERY (0) /* opcode - 4 bits */
#define DNS_INVQUERY (1 << 11)
#define DNS_SRVSTATUS (1 << 12)
#define DNS_AA (1 << 10) /* authoritative answer */
#define DNS_TC (1 << 9) /* truncated */
#define DNS_RD (1 << 8) /* recursion desired */
#define DNS_RA (1 << 7) /* recursion available */

/* query classes */
#define DNS_INET 1

/* DNS query types */
#define DNS_A 1 /* name -> IP */
#define DNS_NS 2 /* name server */
#define DNS_CNAME 5 /* canonical name */
#define DNS_PTR 12 /* IP->name */
#define DNS_HINFO 13 /* host info/SOA */
#define DNS_MX 15 /* mail exchange */
#define DNS_AXFR 252 /* request for zone transfer */
#define DNS_ANY 255 /* all records */


#define DNS_OK 0 /* rcode = reply codes */
#define DNS_FORMAT 1 /* format error (unable to interpret) */
#define DNS_SERVERFAIL 2 /* server failure */
#define DNS_ERROR 3 /* no DNS entry */
#define DNS_NOTIMPL 4 /* not implemented */
#define DNS_REFUSED 5 /* server refused the query */

#define RCODE_MASK 0xF

#define BIT(x) (1 << x) 
#define COMPRESSION_MASK (~(BIT(15)|BIT(14)))

#pragma pack(push,1)
class queryHeader 
{
public:
	unsigned short type;
	unsigned short dns_class;
};

class fixedDNSheader 
{
public:
	unsigned short ID;
	unsigned short flags;
	unsigned short questions;
	unsigned short answers;
	unsigned short name_server_records;
	unsigned short add_records;
};

class fixedRR
{
public:
	unsigned short type;
	unsigned short dns_class;
	int TTL;
};

#pragma pack(pop)

struct statistic
{
	unsigned long total_queries;
	unsigned long sucess;
	unsigned long no_dns_record;
	unsigned long no_auth_server;
	unsigned long local_dns_timeout;
	vector<unsigned long>delays;
	vector<unsigned long>retx;
};

class Batch_resolver
{
private:
	CRITICAL_SECTION f_lock;
	ifstream f;
	ofstream fo;
public:
	Batch_resolver(const char* filename){InitializeCriticalSection(&f_lock);f.open(filename);fo.open("DNS_answer.txt");}
	void run(int nthreads);
	~Batch_resolver(){DeleteCriticalSection(&f_lock);f.close();fo.flush();fo.close();}
};



class DNS
{
private:
	Socket s;
	unsigned long seq_num;
public:
	DNS():seq_num(0){};
    string resolve(string hostname);
	pair<int,int> makePkt(string hostname,char* buf);
	bool print_result(const char* buf, int offset);
	pair<int,int> makePkt(string hostname,char* buf,unsigned short query_type);
	static bool is_Ip_addr(string question);
	static bool is_valiad_IP(string ip);
	static void makeDNSquestion(char* question, vector<string>& tokens);
    static int get_domain(char* msg_start, char* name_start, char* domain);
    static int print_RR(char* start, int offset,int number);
	bool make_result_string(const char* buf, int offset,char* result);
	int make_RR_string(char* start, int offset, int number,char*& rr);
    static int get_RR_size(char* start, int offset,int number);
	static vector<string> getDNSServer(void);
};


extern bool is_integer(string s);
extern vector<string> split(string& s, char seperator);


