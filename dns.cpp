/*
	KE Wang
	822002009
*/
#pragma comment (lib,"Iphlpapi.lib")
#include "stdafx.h"

CRITICAL_SECTION statistic_lock;
statistic stat;

bool is_integer(string s)
{
	for(int i = 0; i < s.size(); i++)
		if(s[i] > '9' || s[i] < '0')
			return false;
	return true;
}

vector<string> split(string& s, char seperator)
{
	stringstream ss(s);
	string token;
	vector<string> result;
	while(!ss.eof())
	{
		std::getline(ss,token,seperator);
		result.push_back(token);
	}
	return result;
}

bool DNS::is_Ip_addr(string question)
{
	for(int i = 0; i < question.size(); i++)
	{
		if(('0' <= question[i] && question[i] <= '9') || (question[i] == '.'))
			continue;
		else return false;
	}
	return true;
}

bool DNS::is_valiad_IP(string ip)
{
	if(inet_addr(ip.c_str()) == INADDR_NONE)
		return false;
}


vector<string> DNS::getDNSServer(void)
{
	// MSDN sample code
	FIXED_INFO *FixedInfo;
	ULONG    ulOutBufLen;
	DWORD    dwRetVal;
	IP_ADDR_STRING * pIPAddr;

	ulOutBufLen = sizeof(FIXED_INFO);
	FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, sizeof( FIXED_INFO ) );
	ulOutBufLen = sizeof( FIXED_INFO );

	if(ERROR_BUFFER_OVERFLOW == GetNetworkParams(FixedInfo, &ulOutBufLen)) {
		GlobalFree( FixedInfo );
		FixedInfo = (FIXED_INFO *)GlobalAlloc( GPTR, ulOutBufLen );
	}

	if ( dwRetVal = GetNetworkParams( FixedInfo, &ulOutBufLen ) ) {
		printf( "Call to GetNetworkParams failed. Return Value: %08x\n", dwRetVal );
	}
	else {
		//printf( "Host Name: %s\n", FixedInfo->HostName );
		//printf( "Domain Name: %s\n", FixedInfo->DomainName );

		//printf( "DNS Servers:\n" );
		//printf( "\t%s\n", FixedInfo->DnsServerList.IpAddress.String);
	    
		vector<string> hosts;
		hosts.push_back(FixedInfo->DnsServerList.IpAddress.String);
		pIPAddr = FixedInfo->DnsServerList.Next;
		while ( pIPAddr ) {
			hosts.push_back(pIPAddr ->IpAddress.String);
			//printf( "\t%s\n", pIPAddr ->IpAddress.String);
			pIPAddr = pIPAddr ->Next;
		}	
		return hosts;
	}

	GlobalFree (FixedInfo);
}


void DNS::makeDNSquestion(char* question, vector<string>& tokens)
{
	char* pos = question;
	for(auto e : tokens)
	{
		*pos++ = e.size();
		memcpy(pos,e.c_str(),e.size());
		pos += e.size();
	}
	*pos = '\0';
}

int DNS::get_domain(char* msg_start, char* name_start, char* domain)
{
	int buf_offset = 0;
	int domain_offset = 0;
	unsigned char len = 0; 
	unsigned char count = 0;
    int result_offset = 0;
	bool is_compression = false;

	do
	{	
		if(name_start[buf_offset] == 0)
		{
			result_offset += sizeof(unsigned short);
			return result_offset;
		}

		if((name_start[buf_offset] & (BIT(7)|BIT(6))) == (BIT(7)|BIT(6)))/*compression*/
		{
			unsigned short* pname = (unsigned short*)(name_start + buf_offset);/*sequence problem?*/
			unsigned short pointer_offset = ntohs(*pname) & COMPRESSION_MASK;
			name_start = msg_start + pointer_offset;
			if(!is_compression)
			{
			    is_compression = true;
			    result_offset = buf_offset + sizeof(unsigned short);
			}
		    buf_offset = 0;
		}
		
		len = name_start[buf_offset++];
		while(count < len)
		{
			domain[domain_offset++] = name_start[buf_offset++];
			count++;
		}
		if(name_start[buf_offset] == '\0')
		{
			domain[domain_offset] = '\0';
			if(is_compression)
			{
				return result_offset;
			}
			else
			{
				result_offset = buf_offset + 1;
				return result_offset;
			}				
		}
		domain[domain_offset++] = '.';
		count = 0;	
	}
	while(1);
}

int DNS::make_RR_string(char* start, int offset, int number,char*& rr)
{
	char* field_start = start + offset;
	int RR_offset = 0;
	if(number == 0)
		return RR_offset;
	for(int i = 0;i < number; i++)
	{
		unique_ptr<char> domain(new char[256]);
		RR_offset += get_domain(start,field_start + RR_offset,domain.get());
		rr += sprintf(rr,"%s is ",domain.get());

		fixedRR* frr = (fixedRR*) (field_start + RR_offset);
		unsigned short DNS_class;
		unsigned short type;
		int ttl;
		unsigned short RDlen = 0;

		DNS_class = ntohs(frr->dns_class);
		type = ntohs(frr->type);
		ttl = ntohl(frr->TTL);

		RR_offset += sizeof(fixedRR);
		if(frr->dns_class != 1)
		{
		    RDlen = ntohs(*(unsigned short*)(field_start + RR_offset));
		}
		else
		{
		    RDlen = (*(unsigned short*)(field_start + RR_offset));
		}
		
		RR_offset += sizeof(unsigned short);
		if(type == DNS_A)
		{
			string name;
			unsigned long ip = *(unsigned long*)(field_start + RR_offset);
			IN_ADDR in_ip;
			in_ip.s_addr = ip;
			name = inet_ntoa(in_ip);
			rr += sprintf(rr,"%s\n",name.c_str());
			RR_offset += sizeof(unsigned long);
		}
		if(type == DNS_CNAME)
		{
			unique_ptr<char> domain(new char[256]);
            RR_offset += get_domain(start,field_start + RR_offset,domain.get());
			rr += sprintf(rr,"alised to %s\n",domain.get());
		}
		if(type == DNS_PTR)
		{
			unique_ptr<char> domain(new char[256]);
            RR_offset += get_domain(start,field_start + RR_offset,domain.get());
			rr += sprintf(rr,"%s\n",domain.get());
		}	
	}
	return RR_offset;
}


int DNS::get_RR_size(char* start, int offset,int number)
{
	char* field_start = start + offset;
	int RR_offset = 0;
	if(number == 0)
		return RR_offset;
	for(int i = 0;i < number; i++)
	{
		unique_ptr<char> domain(new char[256]);
		RR_offset += get_domain(start,field_start + RR_offset,domain.get());

		fixedRR* frr = (fixedRR*) (field_start + RR_offset);
		unsigned short DNS_class;
		unsigned short type;
		int ttl;
		unsigned short RDlen = 0;
		if(frr->dns_class != 1)
		{
		    DNS_class = ntohs(frr->dns_class);
			type = ntohs(frr->type);
			ttl = ntohl(frr->TTL);
		}
		else
		{
			DNS_class = (frr->dns_class);
			type = (frr->type);
			ttl = (frr->TTL);
		}

		RR_offset += sizeof(fixedRR);
		if(frr->dns_class != 1)
		{
		    RDlen = ntohs(*(unsigned short*)(field_start + RR_offset));
		}
		else
		{
		    RDlen = (*(unsigned short*)(field_start + RR_offset));
		}
		
		RR_offset += sizeof(unsigned short);
		if(type == DNS_A)
		{
			string name;
			unsigned long ip = *(unsigned long*)(field_start + RR_offset);
			IN_ADDR in_ip;
			in_ip.s_addr = ip;
			name = inet_ntoa(in_ip);
			RR_offset += sizeof(unsigned long);
		}
		else
		{
			unique_ptr<char> domain(new char[256]);
            RR_offset += get_domain(start,field_start + RR_offset,domain.get());
		}	
	}
	return RR_offset;
}

bool DNS::make_result_string(const char* buf, int offset,char* result)
{
	fixedDNSheader * fdh = (fixedDNSheader*) buf;
	unsigned short Id = ntohs(fdh->ID);
	if(seq_num != Id)
		return false;
	unsigned short flags = ntohs(fdh->flags);
	unsigned short answer_count = ntohs(fdh->answers);
	unsigned short authority_count = ntohs(fdh->name_server_records);
	unsigned short addition_count = ntohs(fdh->add_records);
	char* pos = result;

	if((flags & RCODE_MASK) == DNS_SERVERFAIL)
	{
		EnterCriticalSection(&statistic_lock);
		stat.no_auth_server++;
		LeaveCriticalSection(&statistic_lock);
		pos += sprintf(pos,"Authoritative DNS server not found\n");
		return false;
	}
	else if((flags & RCODE_MASK) == DNS_ERROR)
	{
		EnterCriticalSection(&statistic_lock);
		stat.no_dns_record++;
		LeaveCriticalSection(&statistic_lock);
		pos += sprintf(pos,"No DNS entry\n");
		return false;
	}
	else if((flags & RCODE_MASK) != DNS_OK)
	{
		pos += sprintf(pos,"Some error = %d\n",(flags & RCODE_MASK));
		return false;
	}
	
	EnterCriticalSection(&statistic_lock);
	stat.sucess++;
	LeaveCriticalSection(&statistic_lock);

	int RR_offset = 0;
	pos += sprintf(pos,"Answer(s) :\n");
	int answer_offset = offset;
	RR_offset += make_RR_string((char*)buf,answer_offset,answer_count,pos);

	int autority_offset = answer_offset + RR_offset;
	RR_offset += get_RR_size((char*)buf,autority_offset,authority_count);

	if(addition_count == 0)
	{	
		pos += sprintf(pos,"\n");
		return true;
	}
	pos += sprintf(pos,"Additional Answer(s) :\n");
	int additional_offset = autority_offset + RR_offset;
	RR_offset += make_RR_string((char*)buf,answer_offset,answer_count,pos);
	pos += sprintf(pos,"\n");

	return true;

}


pair<int,int> DNS::makePkt(string hostname,char* buf,unsigned short query_type)
{
	vector<string> tokens;
	tokens = split(hostname,'.');
	int pkt_size = 0;	
	int question_size = 0;
	for(int i = 0; i < tokens.size(); i++)
		question_size += tokens[i].size() + 1;
	question_size += 1 + sizeof(queryHeader);
	pkt_size += sizeof(fixedDNSheader) + question_size;
		
	fixedDNSheader* dns_header = (fixedDNSheader*) buf;
	queryHeader* query_header = (queryHeader*)(buf + pkt_size - sizeof(queryHeader));

	dns_header->ID = htons(++seq_num);
	dns_header->flags = htons(DNS_STDQUERY | DNS_RD);
	dns_header->questions = htons(1);
	dns_header->add_records = 0;
	dns_header->answers = 0;
	dns_header->name_server_records = 0;

	query_header->dns_class = htons(DNS_INET);
	query_header->type = htons(query_type);

	makeDNSquestion((char*)dns_header + sizeof(fixedDNSheader), tokens);
	return make_pair(pkt_size,question_size);
}

string DNS::resolve(string hostname)
{
	unsigned short query_type;
	if(is_Ip_addr(hostname))
	{
		if(is_valiad_IP(hostname))
		{	
			query_type = DNS_PTR;
		}
		else
		{
			printf("Invalid IP address\n");
			return "";
		}	
	}
	else
	{
		query_type = DNS_A;
	}
	
	unique_ptr<char> buf(new char[1024]);
	pair<int,int> size_pair = makePkt(hostname,buf.get(),query_type);
	
	int pkt_size = size_pair.first;
	int question_size = size_pair.second;
	int count = 0;
		
	vector<string> server_ips = getDNSServer();
	while(count++ < 3)
	{
		string server_ip = server_ips[count%server_ips.size()];
		if(!s.sock_send(buf.get(),pkt_size,(char*)server_ip.c_str(),"53"))
			continue;

		if(s.sock_recv())
			break;
	}
	
	EnterCriticalSection(&statistic_lock);
	stat.retx.push_back(count);
	LeaveCriticalSection(&statistic_lock);
	
	string result;
	unique_ptr<char> str_buf(new char[4096]);
	if(count >= 3)
	{
		EnterCriticalSection(&statistic_lock);
		stat.local_dns_timeout++;
		LeaveCriticalSection(&statistic_lock);
		sprintf(str_buf.get(),"Local DNS timeout\n");
		result = (str_buf.get());
	    return result;
	}


	make_result_string(s.get_recv_buf(),sizeof(fixedDNSheader) + question_size,str_buf.get());
	result = (str_buf.get());
	return result;
}

class parameter
{
public:
	queue<string>* questions;
	queue<string>* answers;
	CRITICAL_SECTION* cs;
};

int c;
DWORD WINAPI batch_handler(void* param)
{
	DNS dns; 
	queue<string>* questions;
	queue<string>* answers;
	parameter* p = (parameter*)param;
	CRITICAL_SECTION* cs = p->cs;
	questions = p->questions;
	answers = p->answers;
	string question;
	string answer;
	while(1)
	{
		EnterCriticalSection(cs);
		c++;
		if(c>100000)
		{
			LeaveCriticalSection(cs);
			return 0;
		}
		question = questions->front();
		questions->pop();
		LeaveCriticalSection(cs);
		
		/*
	    EnterCriticalSection(&statistic_lock);
	    stat.total_queries++;
	    LeaveCriticalSection(&statistic_lock);
		unsigned long start = GetTickCount();
		if((gethostbyname(question.c_str())) != NULL)
		{
			//printf("Get host by name with error = %d\n",WSAGetLastError());
			EnterCriticalSection(&statistic_lock);
	        stat.sucess++;
	        LeaveCriticalSection(&statistic_lock);
		}
		unsigned long end = GetTickCount();
		EnterCriticalSection(&statistic_lock);
		stat.delays.push_back(end - start);
		LeaveCriticalSection(&statistic_lock);
		*/
		unsigned long start = GetTickCount();
		answer = dns.resolve(question);
		unsigned long end = GetTickCount();
		
		EnterCriticalSection(&statistic_lock);
		stat.delays.push_back(end - start);
		LeaveCriticalSection(&statistic_lock);
		
		EnterCriticalSection(cs);
		if(answer != "")
			answers->push(answer);		
		LeaveCriticalSection(cs);
	}
	
	return 0;
}


void init_stat()
{
	stat.local_dns_timeout = 0;
	stat.no_auth_server = 0;
	stat.no_dns_record = 0;
	stat.sucess = 0;
	stat.total_queries = 0;
}

void Batch_resolver::run(int nthreads)
{
	printf("Starting batch mode with %d threads...\n",nthreads);
	
	init_stat();
	HANDLE* handles = (HANDLE*)malloc(sizeof(HANDLE) * nthreads);
	//vector<HANDLE> handles;
	parameter p; 
	queue<string> questions;
	queue<string> answers;
	unsigned long entries = 0;
	while(!f.eof())
	{
		string line;
		getline(f,line);
		questions.push(line);
		entries++;
		//printf("%s",line.c_str());
	}
	printf("Reading input file... found %d entries\n",entries);
	printf("...\n");
	int start = GetTickCount();
	p.questions = &questions;
	p.answers = &answers;
	p.cs = &this->f_lock;
    for(int i=0;i<nthreads;i++)
	{
	    handles[i] = CreateThread(NULL,0,batch_handler,&p,0,0);
		if(handles[i] == NULL)
		{
			printf("Cant create more thread = %d",GetLastError());
			break;
		}
		//handles.push_back(R_handle);
	}
	
	for(int i=0; i<= nthreads/MAXIMUM_WAIT_OBJECTS;i++)
	{
		if(i<nthreads/MAXIMUM_WAIT_OBJECTS)
		{
			if(WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS,handles + i*MAXIMUM_WAIT_OBJECTS,true, INFINITE) == -1)
			{
				printf("WaitForMultipleObjects = %d",GetLastError());
				return;
			}
		}
		else
		{
		    if(WaitForMultipleObjects(nthreads - i*MAXIMUM_WAIT_OBJECTS,handles + i*MAXIMUM_WAIT_OBJECTS,true, INFINITE) == -1)
			{
				printf("WaitForMultipleObjects = %d",GetLastError());
				return;
			}
		}
	}

	int end = GetTickCount();
	
	CPU cpu;
	Sleep (500);
	double util = cpu.GetCpuUtilization (NULL);

	printf("Completed %d queries\n",entries);
	printf("       Successful: %f%%\n",100 * (double)stat.sucess/stat.total_queries);
	printf("       No DNS record: %f%%\n",100*(double)stat.no_dns_record/stat.total_queries);
	printf("       No auth DNS server: %f%%\n",100*(double)stat.no_auth_server/stat.total_queries);
	printf("       Local DNS timeout: %f%%\n",100*(double)stat.local_dns_timeout/stat.total_queries);
	
	ofstream o("result.csv");
	o<<nthreads<<","<<stat.total_queries<<","<<stat.sucess<<","<<100 * (double)stat.sucess/stat.total_queries<<","<<(end-start)/1000<<","<<(double)stat.total_queries/((end-start)/1000)<<","<<util<<"\n";
	o.flush();
    o.close();
	
	int sum_delay = 0;
	ofstream o1("delay.csv");
	for(auto e: stat.delays)
	{
		sum_delay += e;
		o1<<e<<"\n";
	}
	o1.flush();
    o1.close();
	printf("       Average delay: %fms\n",(double)sum_delay/stat.delays.size());
	int sum_rx = 0;
	for(auto e: stat.retx)
	{
		sum_rx += e;
	}
	printf("       Average retx attempts: %f\n",(double)sum_rx/stat.retx.size());
	printf("Writing output file... finished with %d answers",answers.size());

	while(!answers.empty())
	{
		fo<<answers.front();
        answers.pop();
	}
	
}
