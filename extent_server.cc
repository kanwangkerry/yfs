// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
	pthread_mutex_init(&extent_server_m, NULL);
	int r;
	put(0x00000001, "", r);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
	// You fill this in for Lab 2.

	pthread_mutex_lock(&extent_server_m);

	struct extent_protocol::attr temp_a;
	unsigned int current_time = time(NULL);
	if(content.count(id) != 0){
		temp_a = attribute[id];
		printf("file key already exists: %lld\n", id);
	}
	content[id] = buf;
	temp_a.ctime = current_time;
	temp_a.mtime = current_time;
	temp_a.size = buf.size();
	attribute[id] = temp_a;
	
	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
	// You fill this in for Lab 2.
	// lock mutex because we modify the access time

	unsigned int current_time = time(NULL);	
	struct extent_protocol::attr temp_a;
	if(content.count(id) == 0){
		printf("file key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);

	buf = content[id];
	temp_a = attribute[id];
	temp_a.atime = current_time;
	attribute[id] = temp_a;

	pthread_mutex_unlock(&extent_server_m);

	return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
	// You fill this in for Lab 2.
	// You replace this with a real implementation. We send a phony response
	// for now because it's difficult to get FUSE to do anything (including
	// unmount) if getattr fails.
	if(content.count(id) == 0){
		printf("file key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}
	a = attribute[id];
	return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
	// You fill this in for Lab 2.

	if(content.count(id) == 0){
		printf("file key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);

	content.erase(id);
	attribute.erase(id);

	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}

int
extent_server::get_dir(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t &file_id)
{
	// You fill this in for Lab 2.
	// lock mutex because we modify the access time

	unsigned int current_time = time(NULL);	
	struct extent_protocol::attr temp_a;
	if(dir_content.count(id) == 0){
		printf("dir key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}
	if(dir_content[id].count(name) == 0){
		printf("dir key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);

	file_id = dir_content[id][name];
	temp_a = attribute[id];
	temp_a.atime = current_time;
	attribute[id] = temp_a;

	pthread_mutex_unlock(&extent_server_m);

	return extent_protocol::OK;
}

/**
 * put_dir is put a file "name" into the id dir
 */

int
extent_server::put_dir(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t &file_id)
{
	// You fill this in for Lab 2.

	pthread_mutex_lock(&extent_server_m);

	struct extent_protocol::attr temp_a;
	unsigned int current_time = time(NULL);
	if(dir_content.count(id) != 0){
		temp_a = attribute[id];
	}
	else{
		std::map<std::string, extent_protocol::extentid_t> temp_map;
		dir_content[id] = temp_map;
	}
	if(dir_content[id].count(name) != 0){
		printf("file name already exists: %s in dir %lld\n", name.c_str(), id);
	}
	file_id =  file_id & 0x000000007FFFFFFF;
	file_id = file_id | 0x0000000080000000;
	while(attribute.count(file_id)!=0){
		file_id ++;
		file_id = file_id & 0x000000007fffffff;
		file_id = file_id | 0x0000000080000000;
	}
	long long unsigned temp = file_id;
	temp =  temp & 0x00000000FFFFFFFF;
	file_id = temp | 0x0000000080000000;
	dir_content[id][name] = file_id;
	temp_a.ctime = current_time;
	temp_a.mtime = current_time;
	attribute[id] = temp_a;
	
	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}


//this function is put a dir into a id directory
int
extent_server::make_dir(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t &file_id)
{
	// You fill this in for Lab 2.

	pthread_mutex_lock(&extent_server_m);

	struct extent_protocol::attr temp_a;
	unsigned int current_time = time(NULL);
	if(dir_content.count(id) != 0){
		temp_a = attribute[id];
	}
	else{
		std::map<std::string, extent_protocol::extentid_t> temp_map;
		dir_content[id] = temp_map;
	}
	if(dir_content[id].count(name) != 0){
		printf("file name already exists: %s in dir %lld\n", name.c_str(), id);
	}
	printf("id: %016llx\n", file_id);
	file_id =  file_id & 0x000000007FFFFFFF;
	while(attribute.count(file_id)!=0){
		file_id ++;
		file_id = file_id & 0x000000007fffffff;
	}
	file_id = file_id & 0x000000007fffffff;
	printf("id: %016llx\n", file_id);
	dir_content[id][name] = file_id;
	temp_a.ctime = current_time;
	temp_a.mtime = current_time;
	attribute[id] = temp_a;

	std::map<std::string, extent_protocol::extentid_t> temp_map;
	dir_content[file_id] = temp_map;
	
	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}

std::string get_filename(extent_protocol::extentid_t id)
{
  std::ostringstream ost;
  ost << id;
  return ost.str();
}

//add empty hole NULL into the returned string, as a delimeter
int
extent_server::read_dir_name(extent_protocol::extentid_t id, std::string &dir_name)
{
	// You fill this in for Lab 2.
	// lock mutex because we modify the access time

	unsigned int current_time = time(NULL);	
	struct extent_protocol::attr temp_a;
	if(dir_content.count(id) == 0){
		printf("dir key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);

	dir_name = "";
	for (std::map<std::string,extent_protocol::extentid_t>::iterator it=dir_content[id].begin(); it!=dir_content[id].end(); ++it){
		dir_name = dir_name + it->first + '\0';
	}

	temp_a = attribute[id];
	temp_a.atime = current_time;
	attribute[id] = temp_a;

	pthread_mutex_unlock(&extent_server_m);

	return extent_protocol::OK;
}

int
extent_server::read_dir_id(extent_protocol::extentid_t id, std::string &dir_id)
{
	// You fill this in for Lab 2.
	// lock mutex because we modify the access time

	unsigned int current_time = time(NULL);	
	struct extent_protocol::attr temp_a;
	if(dir_content.count(id) == 0){
		printf("dir key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);

	dir_id = "";
	for (std::map<std::string,extent_protocol::extentid_t>::iterator it=dir_content[id].begin(); it!=dir_content[id].end(); ++it){
		dir_id = dir_id + get_filename(it->second) + '\0';
	}

	temp_a = attribute[id];
	temp_a.atime = current_time;
	attribute[id] = temp_a;

	pthread_mutex_unlock(&extent_server_m);

	return extent_protocol::OK;
}

int extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr a, int &)
{
	// You fill this in for Lab 2.
	// You replace this with a real implementation. We send a phony response
	// for now because it's difficult to get FUSE to do anything (including
	// unmount) if getattr fails.
	if(content.count(id) == 0){
		printf("file key not exists: %lld\n", id);
		return extent_protocol::NOENT;
	}

	pthread_mutex_lock(&extent_server_m);
	//TODO: only set the size now
	
	unsigned int current_time = time(NULL);	
	//if current size if smaller, set the appending '\0's, or truncating
	if(a.size < attribute[id].size){
		content[id] = content[id].substr(0, a.size);
	}
	else{
		for(unsigned int i = 0 ; i < a.size - attribute[id].size ; i++){
			content[id].push_back('\0');
		}
	}
	attribute[id].size = a.size;
	attribute[id].mtime = current_time;
	attribute[id].ctime= current_time;
	
	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}

int
extent_server::unlink_file(extent_protocol::extentid_t id, std::string name, extent_protocol::extentid_t file_id, int &)
{
	// You fill this in for Lab 3.

	pthread_mutex_lock(&extent_server_m);

	struct extent_protocol::attr temp_a;
	unsigned int current_time = time(NULL);
	if(dir_content.count(id) != 0){
		temp_a = attribute[id];
	}
	else{
		printf("dir not exists: dir %lld\n", id);
		return extent_protocol::NOENT;
	}
	if(dir_content[id].count(name) != 0){
		printf("file not exists: dir %lld, file %s\n", id, name.c_str());
	}

	//remove from the dir
	dir_content[id].erase(name);
	temp_a.ctime = current_time;
	temp_a.mtime = current_time;
	attribute[id] = temp_a;

	//if file_id is dir, also remove the dir_content
	if((file_id & 0x0000000080000000) == 0){
		dir_content.erase(file_id);
	}
	
	pthread_mutex_unlock(&extent_server_m);
	return extent_protocol::OK;
}
