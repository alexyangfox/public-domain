#ifndef IP_MISC_H__ALREADY_INCLUDED__
#define IP_MISC_H__ALREADY_INCLUDED__


extern	int	get_inaddr(struct sockaddr_in* /*addr*/,
			   const char* /*host*/,
			   const char* /*service*/,
			   const char* /*protocol*/);

extern	int	tcp_connect(const struct sockaddr_in* /*remote*/,
			    const struct sockaddr_in* /*local*/);

extern	int	tcp_open(const char* /*remote_host*/,
			 const char* /*remote_port*/,
			 const char* /*local_host*/,
			 const char* /*local_port*/);

extern	int	tcp_listen_2(const struct sockaddr_in* /*local*/);

extern	int	tcp_listen(const char* /*interface*/,
			   const char* /*port*/);

extern	char   *source_route(int /*type*/,
			     char** /*hostlist*/,
			     int /*nhosts*/,
			     int* /*result_len*/);


#endif	/* IP_MISC_H__ALREADY_INCLUDED__ */
