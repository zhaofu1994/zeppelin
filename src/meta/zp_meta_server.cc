#include <glog/logging.h>

#include "zp_meta_server.h"

ZPMetaServer::ZPMetaServer(const ZPOptions& options)
  : options_(options) {

  pthread_rwlock_init(&state_rw_, NULL);

  // Convert ZPOptions
  floyd::Options* fy_options = new floyd::Options();
  fy_options->seed_ip = options.seed_ip;
  fy_options->seed_port = options.seed_port + kMetaPortShiftFY;
  fy_options->local_ip = options.local_ip;
  fy_options->local_port = options.local_port + kMetaPortShiftFY;
  fy_options->data_path = options.data_path;
  fy_options->log_path = options.log_path;

  floyd_ = new floyd::Floyd(*fy_options);  
}

ZPMetaServer::~ZPMetaServer() {
  pthread_rwlock_destroy(&state_rw_);
}

Status ZPMetaServer::Start() {
  LOG(INFO) << "ZPMetaServer started on port:" << options_.local_port << ", seed is " << options_.seed_ip.c_str() << ":" <<options_.seed_port;
  floyd_->Start();
  server_mutex_.Lock();
  server_mutex_.Lock();
  return Status::OK();
}

void ZPMetaServer::CheckNodeAlive() {
  slash::MutexLock l(&alive_mutex_);
  struct timeval now;
  gettimeofday(&now, NULL);
  NodeAliveMap::iterator it = node_alive_.begin();
  for (; it != node_alive_.end(); ++it) {
    if (now.tv_sec - (it->second).tv_sec > NODE_ALIVE_LEASE) {
      update_thread_.ScheduleUpdate(it->first, ZPMetaUpdateOP::OP_REMOVE);
    }
  }
}

Status ZPMetaServer::Set(const std::string &key, const std::string &value) {
  floyd::Status fs = floyd_->Write(key, value);
	if (fs.ok()) {
    return Status::OK();
  } else {
    return Status::Corruption("floyd set error!");
  }
}

Status ZPMetaServer::Get(const std::string &key, std::string &value) {
  floyd::Status fs = floyd_->DirtyRead(key, value);
	if (fs.ok()) {
    return Status::OK();
  } else {
    return Status::Corruption("floyd get error!");
  }
}
