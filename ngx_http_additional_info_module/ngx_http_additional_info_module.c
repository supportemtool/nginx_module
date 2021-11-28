#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdio.h>

static char* ngx_http_additional_info(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_additional_info_handler(ngx_http_request_t* r);
static void get_config_dump(char* ans, ngx_uint_t* len);

static ngx_command_t ngx_http_additional_info_commands[] = {

	{ngx_string("additional_info"),       /* directive */
	 NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, /* location context and takes no arguments*/
	 ngx_http_additional_info,            /* configuration setup function */
	 0,                                   /* No offset. Only one context is supported. */
	 0,                                   /* No offset when storing the module configuration on struct. */
	 NULL},

	ngx_null_command /* command termination */
};

/* The module context. */
static ngx_http_module_t ngx_http_additional_info_module_ctx = {
	NULL, /* preconfiguration */
	NULL, /* postconfiguration */

	NULL, /* create main configuration */
	NULL, /* init main configuration */

	NULL, /* create server configuration */
	NULL, /* merge server configuration */

	NULL, /* create location configuration */
	NULL  /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_additional_info_module = {
	NGX_MODULE_V1,
	&ngx_http_additional_info_module_ctx, /* module context */
	ngx_http_additional_info_commands,    /* module directives */
	NGX_HTTP_MODULE,                      /* module type */
	NULL,                                 /* init master */
	NULL,                                 /* init module */
	NULL,                                 /* init process */
	NULL,                                 /* init thread */
	NULL,                                 /* exit thread */
	NULL,                                 /* exit process */
	NULL,                                 /* exit master */
	NGX_MODULE_V1_PADDING };

static ngx_int_t ngx_http_additional_info_handler(ngx_http_request_t* r)
{
	ngx_buf_t* b;
	ngx_chain_t out;

	// Set the Content-Type header.
	r->headers_out.content_type.len = sizeof("text/plain") - 1;
	r->headers_out.content_type.data = (u_char*)"text/plain";

	// Allocate a new buffer for sending out the reply.
	b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

	out.buf = b;
	out.next = NULL; // just one buffer
	ngx_uint_t len = 0;
	char buff[5120];
	get_config_dump(buff, &len);
	b->pos = (u_char*)buff;
	b->last = b->pos + len - 1;
	b->memory = 1;
	b->last_buf = 1;

	r->headers_out.status = NGX_HTTP_OK; // 200 status code
	r->headers_out.content_length_n = len - 1;
	ngx_http_send_header(r);
	return ngx_http_output_filter(r, &out);
} // ngx_http_additional_info_handler

static char* ngx_http_additional_info(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t* clcf; /* pointer to core location configuration */
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_additional_info_handler;

	return NGX_CONF_OK;
} // ngx_http_additional_info

size_t sprintf_fix(char* buff, size_t len, char* str)
{
	for (size_t i = 0; i < len; ++i)
	{
		buff[i] = str[i];
	}
	return len;
}

int strEqual(char* str, const char* objectName, int nameLen);
int findObject(char* buf, const char* objectName, int end, int nameLen);
size_t parseObject(char* buf, char* output, int end, int* endIndex);
size_t ParseServerNames(char* inputBuff, char* output, size_t inputLen);

static void get_config_dump(char* ans, ngx_uint_t* len)
{
	ngx_array_t conf_dump = ngx_cycle->config_dump;
	ngx_uint_t i;
	ngx_uint_t nelts = conf_dump.nelts;
	char outputBuff[4096];
	*len = 0;
	if ((int)nelts != 0)
	{
		ngx_conf_dump_t* cd = (ngx_conf_dump_t*)(conf_dump.elts);
		for (i = 0; i < nelts; i++)
		{
			//data
			*len += ParseServerNames((char*)cd[i].buffer->pos, (outputBuff + *len), (cd[i].buffer->last - cd[i].buffer->pos));
		}
	}

	if (*len > 3)
		*len -= 3;
	if (*len == 0)
	{
		*len += sprintf(ans, "{\r\n \"ssl_server_names\" : null\r\n} \r\n");
	}
	else
	{
		size_t tmp = 0;
		tmp += sprintf(ans, "{\r\n \"ssl_server_names\" : [\r\n");
		tmp += sprintf_fix((ans + tmp), *len, outputBuff);
		tmp += sprintf((ans + tmp), "\r\n]\r\n} ");
		*len = tmp;
	}
}

#define LISTEN "listen"
#define SSL "ssl"
#define SERVER_NAME "server_name"
#define SERVER "server"
#define INVALID_INDX -1

int strEqual(char* str, const char* objectName, int nameLen)
{
	for (int i = 0; i < nameLen - 1; i++)
	{
		if (objectName[i] != str[i])
			return 0;
	}
	return 1;
}

int findObject(char* buf, const char* objectName, int end, int nameLen)
{
	for (int i = 0; i < end - (nameLen + 1); i++)
	{
		if (buf[i] == objectName[0] && (buf[i + nameLen] == ' ' || buf[i + nameLen] == '{' || buf[i + nameLen] == '\r' || buf[i + nameLen] == '\n') && strEqual((buf + i), objectName, nameLen))
		{
			return i;
		}
	}
	return INVALID_INDX;
}

size_t parseObject(char* buf, char* output, int end, int* endIndex)
{
	int numServernames = 0;
	int isSSL = 0;
	char listen[1048];
	char ports[1048];
	int lenPorts = 0;
	int numberOfPorts = 0;
	char server_name[20][1048];
	*endIndex = INVALID_INDX;
	for (int i = 0; i < end; i++)
	{
		if (buf[i] == '}')
		{
			*endIndex = i;
			break;
		}
		else
		{
			if (buf[i] == LISTEN[0] && strEqual((buf + i), LISTEN, sizeof(LISTEN)))
			{
				i += sizeof(LISTEN) - 1;
				for (int j = i; j < end; j++)
				{
					int k = 0;
					while ((j < end) && (buf[j] == ' ' || buf[j] == '\n' || buf[j] == '\r'))
						j++;
					if (buf[j] == ';')
						break;
					while (buf[j] != ' ' && buf[j] != ';' && j < end)
					{
						listen[k++] = buf[j++];
					}
					if (j < (int)(end - sizeof(SSL)) && strEqual((listen), SSL, sizeof(SSL)))
					{
						isSSL = 1;
					}
					else
					{
						listen[k] = '\0';
						if (numberOfPorts == 0)
							lenPorts += sprintf((ports + lenPorts), "%s", (char*)listen);
						else
							lenPorts += sprintf((ports + lenPorts), ",%s", (char*)listen);
						numberOfPorts++;
					}
					if (buf[j] == ';')
						break;


				}
			}

			if (buf[i] == SERVER_NAME[0] && strEqual((buf + i), SERVER_NAME, sizeof(SERVER_NAME)))
			{
				i += sizeof(SERVER_NAME) - 1;
				for (int j = i; j < end; j++)
				{
					int k = 0;
					while ((j < end) && (buf[j] == ' ' || buf[j] == '\n' || buf[j] == '\r'))
						j++;
					if (buf[j] == ';')
						break;
					while (buf[j] != ' ' && buf[j] != '\r' && buf[j] != '\n' && buf[j] != ';' && j < end)
					{
						server_name[numServernames][k++] = buf[j++];
					}
					server_name[numServernames][k++] = '\0';
					numServernames++;
					if (buf[j] == ';')
						break;
				}
			}
		}
	}
	if (*endIndex == INVALID_INDX)
		*endIndex = end;
	if (isSSL == 1)
	{
		ports[lenPorts] = '\0';
		size_t index = 0;
		while (numServernames != 0)
		{
			index += sprintf((output + index), "\"%s:%s\",\r\n", (char*)server_name[(--numServernames)], (char*)ports);
		}
		return index;
	}

	return 0;
}

size_t ParseServerNames(char* inputBuff, char* output, size_t inputLen)
{
	size_t outputLen = 0;
	size_t i = 0;
	while (i < inputLen)
	{
		int d = 0;
		d = findObject(inputBuff, SERVER, (inputLen - i), sizeof(SERVER));
		inputBuff += d;
		i += d;
		if (d != INVALID_INDX)
		{
			outputLen += parseObject(inputBuff, (output + outputLen), (inputLen - i), &d);
			inputBuff += d;
			i += d;
		}
		else
		{
			break;
		}
	}
	return outputLen;
}
