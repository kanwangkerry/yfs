// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

std::vector<std::string>
yfs_client::split_file_name(std::string p){
	std::vector<std::string> result;
	unsigned int index = 0, l_index = 0;
	while(index < p.size()){
		l_index = p.find(" ", index);
		if(l_index == std::string::npos){
			break;
		}
		else{
			result.push_back(p.substr(index, l_index - index));
			index = l_index + 1;
		}
	}
	return result;
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

/**
 * Look up @parent the file @name. return error when rpc call get 
 * erro. return NOENT when no matching file @name found.
 */
int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &file_inum){
	std::string parent_buf;
	std::string name_s(name);
	int r = ec->get_dir(parent, name_s, file_inum);
	if(r == extent_protocol::OK){
		found = true;
	}
	else{
		found = false;
	}
	return r;
}

int
yfs_client::create(inum parent, const char *name, inum &file_id)
{
	std::string s_name(name);
	int r = ec->put_dir(parent, name, file_id);
	if(r != extent_protocol::OK)
		return r;

	r = ec->put(file_id, "");
	return r;

}

