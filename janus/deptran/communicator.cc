
#include "communicator.h"
#include "rcc/graph.h"
#include "rcc/graph_marshaler.h"
#include "command.h"
#include "command_marshaler.h"
#include "txn_chopper.h"
#include "rcc_rpc.h"


namespace rococo {

using namespace std::chrono;

Communicator::Communicator(PollMgr* poll_mgr) {
  vector<string> addrs;
  if (poll_mgr == nullptr)
    rpc_poll_ = new PollMgr(1);
  else
    rpc_poll_ = poll_mgr;
  auto config = Config::GetConfig();
  vector<parid_t> partitions = config->GetAllPartitionIds();
  for (auto &par_id : partitions) {
    auto site_infos = config->SitesByPartitionId(par_id);
    vector<std::pair<siteid_t, ClassicProxy*>> proxies;
    for (auto &si : site_infos) {
      auto result = ConnectToSite(si, milliseconds(CONNECT_TIMEOUT_MS));
      verify(result.first == SUCCESS);
      proxies.push_back(std::make_pair(si.id, result.second));
    }
    rpc_par_proxies_.insert(std::make_pair(par_id, proxies));
  }
}

Communicator::~Communicator() {
  verify(rpc_clients_.size() > 0);
  for (auto &pair : rpc_clients_) {
    rrr::Client *rpc_cli = pair.second;
    rpc_cli->close_and_release();
  }
  rpc_clients_.clear();
}

std::pair<siteid_t, ClassicProxy*>
Communicator::RandomProxyForPartition(parid_t par_id) const {
  auto it = rpc_par_proxies_.find(par_id);
  verify(it != rpc_par_proxies_.end());
  auto& par_proxies = it->second;
  int index = rrr::RandomGenerator::rand(0, par_proxies.size() - 1);
  return par_proxies[index];
}

std::pair<siteid_t, ClassicProxy*>
Communicator::LeaderProxyForPartition(parid_t par_id) const {
  auto leader_cache = const_cast<map<parid_t, SiteProxyPair>&>(this->leader_cache_);
  auto leader_it = leader_cache.find(par_id);
  if (leader_it != leader_cache.end()) {
    return leader_it->second;
  } else {
    auto it = rpc_par_proxies_.find(par_id);
    verify(it != rpc_par_proxies_.end());
    auto &partition_proxies = it->second;
    auto config = Config::GetConfig();
    auto proxy_it = std::find_if(partition_proxies.begin(),
                                 partition_proxies.end(),
                                 [config](const std::pair<siteid_t, ClassicProxy *> &p) {
                                   auto &site = config->SiteById(p.first);
                                   return site.locale_id == 0;
                                 });
    if (proxy_it == partition_proxies.end()) {
      Log_fatal("could not find leader for partition %d", par_id);
    } else {
      leader_cache[par_id] = *proxy_it;
      Log_debug("leader site for parition %d is %d", par_id, proxy_it->first);
    }
    return *proxy_it;
  }
}

std::pair<int, ClassicProxy*>
Communicator::ConnectToSite(Config::SiteInfo &site,
                            milliseconds timeout) {
  string addr = site.GetHostAddr();
  auto start = steady_clock::now();
  rrr::Client* rpc_cli = new rrr::Client(rpc_poll_);
  double elapsed;
  int attempt = 0;
  do {
    Log_info("connect to site: %s (attempt %d)", addr.c_str(), attempt++);
    auto connect_result = rpc_cli->connect(addr.c_str());
    if (connect_result == SUCCESS) {
      ClassicProxy *rpc_proxy = new ClassicProxy(rpc_cli);
      rpc_clients_.insert(std::make_pair(site.id, rpc_cli));
      rpc_proxies_.insert(std::make_pair(site.id, rpc_proxy));
      Log_debug("connect to site: %s success!", addr.c_str());
      return std::make_pair(SUCCESS, rpc_proxy);
    } else {
      std::this_thread::sleep_for(milliseconds(CONNECT_SLEEP_MS));
    }
    auto end = steady_clock::now();
    elapsed = duration_cast<milliseconds>(end - start).count();
  } while(elapsed < timeout.count());
  Log_info("timeout connecting to %s", addr.c_str());
  rpc_cli->close_and_release();
  return std::make_pair(FAILURE, nullptr);
}

std::pair<siteid_t, ClassicProxy*>
Communicator::NearestProxyForPartition(parid_t par_id) const {
  // TODO Fix me.
  auto it = rpc_par_proxies_.find(par_id);
  verify(it != rpc_par_proxies_.end());
  auto& partition_proxies = it->second;
  verify(partition_proxies.size() > loc_id_);
  int index = loc_id_;
  return partition_proxies[index];
};

} // namespace rococo