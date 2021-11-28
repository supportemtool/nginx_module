How to build module
1. Copy ngx_http_additional_info_module to Nginx source code
2. Configure with paremeres --with-debug and add our module ---add-dynamic-module="PATH TO MODULE"
	EXAMPLE ./configure --add-dynamic-module==/etc/nginx/ngx_http_additional_info_module --with-debug .......
3. Run Make
4. Run Make install
5. Copy builded module to Nginx modules folder


How to use module
1. In Nginx configurarion add module
	EXAMPLE "load_module modules/ngx_http_additional_info_module.so;" to 1st line
2. Determinate location
	EXAMPLE  "
		location = /emtool {
		additional_info;
		}
		"
3. Set up basic authorization
	EXAMPLE "
	        server {
                listen 80;
                server_name  my_hostname.com;
                location /emtool {
                        auth_basic "Restricted Content";
                        auth_basic_user_file /etc/nginx/.htpasswd;
                        additional_info;
                }
        }


How to check configuration

Use Curl for test configuration. Example "curl -u username:password your_hostname.com/emtool"
Success code 200
Responce EXAMPLE:
{
 "ssl_server_names" : [
"www.site1.com:443",
"site2.org:8443",
"subdomain.site2.org:9443",
]


support@emtool.org
