
//         Copyright E�in O'Callaghan 2006 - 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "stdAfx.hpp"
#include <functional>

#include "global/wtl_app.hpp"
#include "global/string_conv.hpp"
#include "global/ini_adapter.hpp"

#include "halTorrent.hpp"
#include "halTypes.hpp"
#include "halEvent.hpp"
#include "halSignaler.hpp"

#include "halTorrentInternal.hpp"
#include "halSession.hpp"
//#include "halSessionAlert.hpp"

namespace hal 
{
	libtorrent::session* torrent_internal::the_session_ = 0;
}

namespace hal 
{

bit& bittorrent()
{
	static bit t;
	return t;
}

const PeerDetails& torrent_details::peerDetails() const
{
	if (!peerDetailsFilled_)
	{
		bittorrent().get_all_peer_details(hal::to_utf8(name_), peerDetails_);
		peerDetailsFilled_ = true;
	}
	
	return peerDetails_;
}

const FileDetails& torrent_details::fileDetails() const
{
	if (!fileDetailsFilled_)
	{
		bittorrent().get_all_file_details(hal::to_utf8(name_), fileDetails_);
		fileDetailsFilled_ = true;
	}
	
	return fileDetails_;
}

bool nameLess(const torrent_details_ptr& left, const torrent_details_ptr& right)
{
	return left->state() < right->state();
}

bool torrent_details::less(const torrent_details& r, size_t index) const
{
	switch (index)
	{
	case name_e: return name_ < r.name_;
	case state_e: return state_ < r.state_;

	case speed_down_e: return speed_.first < r.speed_.first;
	case speed_up_e: return speed_.second < r.speed_.second;
	case progress_e: return completion_ < r.completion_;
	case distributed_copies_e: return distributed_copies_ < r.distributed_copies_;

	case remaining_e: 
		{
		boost::int64_t left = totalWanted_-totalWantedDone_;
		boost::int64_t right = r.totalWanted_-r.totalWantedDone_;

		return left < right;
		}

	case total_wanted_e: return totalWanted_ < r.totalWanted_;
	case completed_e: return totalWantedDone_ < r.totalWantedDone_; 

	case uploaded_e: return totalPayloadUploaded_ < r.totalPayloadUploaded_;
	case downloaded_e: return totalPayloadDownloaded_ < r.totalPayloadDownloaded_;

	case peers_e: return peers_ < r.peers_;
	case seeds_e: return seeds_ < r.seeds_;

	case ratio_e: 
		{
		float left = (totalPayloadDownloaded_) 
				? static_cast<float>(totalPayloadUploaded_)
					/ static_cast<float>(totalPayloadDownloaded_)
				: 0;
		
		float right = (r.totalPayloadDownloaded_) 
				? static_cast<float>(r.totalPayloadUploaded_)
					/ static_cast<float>(r.totalPayloadDownloaded_)
				: 0;
		
		return left < right; 
		}

	case eta_e: return estimatedTimeLeft_ < r.estimatedTimeLeft_;
	case tracker: return currentTracker_ < r.currentTracker_;
	case update_tracker_in_e: return updateTrackerIn_ < r.updateTrackerIn_;

	case active_time_e: return active_ < r.active_;
	case seeding_time_e: return seeding_ < r.seeding_;
	case start_time_e: return startTime_ < r.startTime_;
	case finish_time_e: return finishTime_ < r.finishTime_;

	case queue_position_e: return queue_position_ < r.queue_position_;
	case managed_e: return managed_ < r.managed_;

	default: return false; // ???
	};
}

std::wstring torrent_details::to_wstring(size_t index)
{
	switch (index)
	{
	case name_e: return name_;
	case state_e: return state_;

	case progress_e: return (wform(L"%1$.2f%%") % (completion_*100)).str(); 
	case speed_down_e: return (wform(L"%1$.2fkb/s") % (speed_.first/1024)).str(); 
	case speed_up_e: return (wform(L"%1$.2fkb/s") % (speed_.second/1024)).str(); 

	case distributed_copies_e: 
		{
		float copies = distributed_copies_;
		
		if (copies < 0)
			return L"Seeding"; 
		else
			return (hal::wform(L"%1$.2f") % copies).str();	
		}

	case remaining_e: 
		{
		return (wform(L"%1$.2fMB") % (static_cast<float>(totalWanted_-totalWantedDone_)/(1024*1024))).str(); 
		}

	case completed_e: return (wform(L"%1$.2fMB") % (static_cast<float>(totalWantedDone_)/(1024*1024))).str();

	case total_wanted_e: 
		{
		return (wform(L"%1$.2fMB") % (static_cast<float>(totalWanted_-totalWantedDone_)/(1024*1024))).str(); 
		}

	case uploaded_e: 
		{
		return (wform(L"%1$.2fMB") % (static_cast<float>(totalPayloadUploaded_)/(1024*1024))).str(); 
		}

	case downloaded_e: 
		{
		return (wform(L"%1$.2fMB") % (static_cast<float>(totalPayloadDownloaded_)/(1024*1024))).str(); 
		}

	case peers_e: return (wform(L"%1% (%2%)") % connectedPeers_ % peers_).str(); 
	case seeds_e: return (wform(L"%1% (%2%)") % connectedSeeds_ % seeds_).str(); 

	case ratio_e: 
		{
			float ratio = (totalPayloadDownloaded_) 
				? static_cast<float>(totalPayloadUploaded_)
					/ static_cast<float>(totalPayloadDownloaded_)
				: 0;
			
			return (wform(L"%1$.2f") % ratio).str(); 
		}

	case eta_e: 
		{ 
		if (!estimatedTimeLeft_.is_special())
			return hal::from_utf8(
				boost::posix_time::to_simple_string(estimatedTimeLeft_));
		else
			return app().res_wstr(HAL_INF);		
		}	

	case tracker: return currentTracker_;

	case update_tracker_in_e:		
		{ 
		if (!updateTrackerIn_.is_special())
			return from_utf8(
				boost::posix_time::to_simple_string(updateTrackerIn_));
		else
			return app().res_wstr(HAL_INF);		
		}	

	case active_time_e: 
		{
		if (!active_.is_special())
			return from_utf8(
				boost::posix_time::to_simple_string(active_));
		else
			return app().res_wstr(HAL_INF);		
		}

	case seeding_time_e: 
		{ 
		if (!seeding_.is_special())
			return from_utf8(
				boost::posix_time::to_simple_string(seeding_));
		else
			return app().res_wstr(HAL_INF);
		}	

	case start_time_e: 
		{ 
		if (!startTime_.is_special())
			return from_utf8(
				boost::posix_time::to_simple_string(startTime_));
		else
			return app().res_wstr(IDS_NA);
		}		

	case finish_time_e: 		
		{ 
		if (!finishTime_.is_special())
			return from_utf8(
				boost::posix_time::to_simple_string(finishTime_));
		else
			return app().res_wstr(IDS_NA);	
		}		

	case queue_position_e: 
		{
			if (queue_position_ != -1)
				return (wform(L"%1%") % queue_position_).str(); 
			else
				return app().res_wstr(IDS_NA);		
		}

	case managed_e:
		{ 
		if (managed_)
			return L"Yes";
		else
			return L"No"; 	
		}	

	default: return L"(Undefined)"; // ???
	};
}

template<typename torrent_Tptr>
bool torrent_details_compare(torrent_Tptr l, torrent_Tptr r, size_t index = 0, bool cmp_less = true)
{
	if (cmp_less)
		return l->less(*r, index);
	else
		return r->less(*l, index);
}

void torrent_details_manager::sort(size_t column_index, bool cmp_less) const
{
	std::stable_sort(torrents_.begin(), torrents_.end(), 
		bind(&torrent_details_compare<torrent_details_ptr>, _1, _2, column_index, cmp_less));
}

web_seed_or_dht_node_detail::web_seed_or_dht_node_detail() : 
	url(L""), 
	port(-1), 
	type(hal::app().res_wstr(HAL_INT_NEWT_ADD_PEERS_WEB)) 
{}

web_seed_or_dht_node_detail::web_seed_or_dht_node_detail(std::wstring u) : 
	url(u), 
	port(-1), 
	type(hal::app().res_wstr(HAL_INT_NEWT_ADD_PEERS_WEB)) 
{}

web_seed_or_dht_node_detail::web_seed_or_dht_node_detail(std::wstring u, int p) : 
	url(u), 
	port(p), 
	type(hal::app().res_wstr(HAL_INT_NEWT_ADD_PEERS_DHT)) 
{}

bit::bit() :
	pimpl_(new bit_impl())
{}

bit_impl* bit::pimpl()
{
	if (!pimpl_) throw std::runtime_error("bittorrent() accessed after destructer");

	return &*pimpl_;
}

const bit_impl* bit::pimpl() const
{
	if (!pimpl_) throw std::runtime_error("bittorrent() accessed after destructer");

	return &*pimpl_;
}

void bit::shutdown_session()
{
	HAL_DEV_MSG(L"Commence shutdown_session()"); 

	pimpl_.reset();

	HAL_DEV_MSG(L"End shutdown_session()"); 
}

void bit::save_torrent_data()
{
	pimpl()->save_torrent_data();
}

bool bit::create_torrent(const create_torrent_params& params, fs::wpath out_file, progress_callback fn)
{
	return pimpl()->create_torrent(params, out_file, fn);
}

bit::torrent bit::get_wstr(const std::wstring& filename)
{
	return bit::torrent(pimpl()->the_torrents_.get(filename));
}

bool bit::listen_on(std::pair<int, int> const& range)
{
	return pimpl()->listen_on(range);
}

int bit::is_listening_on() 
{
	return pimpl()->is_listening_on();
}

void bit::stop_listening()
{
	pimpl()->stop_listening();
}

bool bit::ensure_dht_on(const hal::dht_settings& dht)
{
	return pimpl()->ensure_dht_on(dht);
}

void bit::ensure_dht_off()
{
	pimpl()->ensure_dht_off();
}

void bit::set_mapping(bool upnp, bool nat_pmp)
{
	pimpl()->set_mapping(upnp, nat_pmp);
}

std::wstring bit::upnp_router_model()
{
	return pimpl()->upnp_router_model();
}

queue_settings bit::get_queue_settings()
{
	return pimpl()->get_queue_settings();
}

void bit::set_queue_settings(const queue_settings& s)
{
	pimpl()->set_queue_settings(s);
}

timeouts bit::get_timeouts()
{
	return pimpl()->get_timeouts();
}

void bit::set_timeouts(const timeouts& t)
{
	pimpl()->set_timeouts(t);
}

void bit::set_session_limits(int maxConn, int maxUpload)
{		
	pimpl()->set_session_limits(maxConn, maxUpload);
}

void bit::set_session_speed(float download, float upload)
{
	pimpl()->set_session_speed(download, upload);
}

bool bit::ensure_ip_filter_on(progress_callback fn)
{
	return pimpl()->ensure_ip_filter_on(fn);
}

void bit::ensure_ip_filter_off()
{
	pimpl()->ensure_ip_filter_off();
}

void bit::set_resolve_countries(bool b)
{
	pimpl()->set_resolve_countries(b);
}

void bit::start_smart_ban_plugin()
{
	pimpl()->start_smart_ban_plugin();
}

void bit::start_ut_pex_plugin()
{
	pimpl()->start_ut_pex_plugin();
}

void bit::start_ut_metadata_plugin()
{
	pimpl()->start_ut_metadata_plugin();
}

void bit::start_metadata_plugin()
{
	pimpl()->start_metadata_plugin();
}

#ifndef TORRENT_DISABLE_ENCRYPTION	

void bit::ensure_pe_on(const pe_settings& pe)
{
	pimpl()->ensure_pe_on(pe);
}

void bit::ensure_pe_off()
{
	pimpl()->ensure_pe_off();
}
#endif

void bit::ip_v4_filter_block(boost::asio::ip::address_v4 first, boost::asio::ip::address_v4 last)
{
	pimpl()->ip_filter_.add_rule(first, last, libt::ip_filter::blocked);
	pimpl()->ip_filter_count();
	pimpl()->ip_filter_changed_ = true;
}

void bit::ip_v6_filter_block(boost::asio::ip::address_v6 first, boost::asio::ip::address_v6 last)
{
	pimpl()->ip_v6_filter_block(first, last);
}

size_t bit::ip_filter_size()
{
	return pimpl()->ip_filter_size();
}

void bit::clear_ip_filter()
{
	pimpl()->clear_ip_filter();
}

bool bit::ip_filter_import_dat(boost::filesystem::path file, progress_callback fn, bool octalFix)
{
	return pimpl()->ip_filter_import_dat(file, fn, octalFix);
}

const SessionDetail bit::get_session_details()
{
	SessionDetail details;
	
	details.port = pimpl()->session_.is_listening() ? pimpl()->session_.listen_port() : -1;
	
	libt::session_status status = pimpl()->session_.status();
	
	details.speed = std::pair<double, double>(status.download_rate, status.upload_rate);
	
	details.dht_on = pimpl()->dht_on_;
	details.dht_nodes = status.dht_nodes;
	details.dht_torrents = status.dht_torrents;
	
	details.ip_filter_on = pimpl()->ip_filter_on_;
	details.ip_ranges_filtered = pimpl()->ip_filter_count_;
	
	return details;
}

void bit::set_session_half_open_limit(int halfConn)
{
	pimpl()->session_.set_max_half_open_connections(halfConn);

	event_log.post(shared_ptr<EventDetail>(new EventMsg(
		hal::wform(L"Set half-open connections limit to %1%.") % pimpl()->session_.max_half_open_connections())));
}

void bit::set_torrent_defaults(const connections& defaults)
{
	pimpl()->default_torrent_max_connections_ = defaults.total;
	pimpl()->default_torrent_max_uploads_ = defaults.uploads;

	event_log.post(shared_ptr<EventDetail>(new EventMsg(
		hal::wform(L"Set torrent connections total %1% and uploads %2%.") 
			% defaults.total % defaults.uploads)));

	pimpl()->default_torrent_download_ = defaults.download_rate;
	pimpl()->default_torrent_upload_ = defaults.upload_rate;

	event_log.post(shared_ptr<EventDetail>(new EventMsg(
		hal::wform(L"Set torrent default rates at %1$.2fkb/s down and %2$.2fkb/s upload.") 
			% defaults.download_rate % defaults.upload_rate)));
}

void bit::add_torrent(wpath file, wpath saveDirectory, bool startStopped, bool managed, allocations alloc, 
		boost::filesystem::wpath moveToDirectory, bool useMoveTo) 
{
	pimpl()->add_torrent(file, saveDirectory, startStopped, managed, alloc, moveToDirectory, useMoveTo);
}

const torrent_details_manager& bit::torrentDetails()
{
	return torrentDetails_;
}

const torrent_details_manager& bit::updatetorrent_details_manager(const wstring& focused, const std::set<wstring>& selected)
{
	try {
	
	mutex_t::scoped_lock l(torrentDetails_.mutex_);	
	
	torrentDetails_.clearAll(l);	
	torrentDetails_.torrents_.reserve(pimpl()->the_torrents_.size());
	
	for (TorrentManager::torrentByName::iterator i=pimpl()->the_torrents_.begin(), e=pimpl()->the_torrents_.end(); i != e; ++i)
	{
		wstring utf8Name = (*i).torrent->name();
		torrent_details_ptr pT = (*i).torrent->get_torrent_details_ptr();
		
		if (selected.find(utf8Name) != selected.end())
		{
			torrentDetails_.selectedTorrents_.push_back(pT);
		}
		
		if (focused == utf8Name)
			torrentDetails_.selectedTorrent_ = pT;
		
		torrentDetails_.torrentMap_[(*i).torrent->name()] = pT;
		torrentDetails_.torrents_.push_back(pT);
	}
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH("Torrent Unknown!", "updatetorrent_details_manager")
	
	return torrentDetails_;
}

void bit::resume_all()
{
	pimpl()->resume_all();
}

void bit::close_all(boost::optional<report_num_active> fn)
{
	pimpl()->close_all(fn);
}

PeerDetail::PeerDetail(libt::peer_info& peerInfo) :
	ipAddress(hal::from_utf8_safe(peerInfo.ip.address().to_string())),
	country(L""),
	speed(std::make_pair(peerInfo.payload_down_speed, peerInfo.payload_up_speed)),
	client(hal::from_utf8_safe(peerInfo.client))
{
	std::vector<wstring> status_vec;
	
#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
	if (peerInfo.country[0] != 0 && peerInfo.country[1] != 0)
		country = (hal::wform(L"(%1%)") % hal::from_utf8_safe(string(peerInfo.country, 2))).str().c_str();
#endif	

	if (peerInfo.flags & libt::peer_info::handshake)
	{
		status_vec.push_back(app().res_wstr(HAL_PEER_HANDSHAKE));
	}		
	else if (peerInfo.flags & libt::peer_info::connecting)
	{
		status_vec.push_back(app().res_wstr(HAL_PEER_CONNECTING));
	}
	else
	{
	#ifndef TORRENT_DISABLE_ENCRYPTION		
		if (peerInfo.flags & libt::peer_info::rc4_encrypted)
			status_vec.push_back(app().res_wstr(HAL_PEER_RC4_ENCRYPTED));		
		if (peerInfo.flags & libt::peer_info::plaintext_encrypted)
			status_vec.push_back(app().res_wstr(HAL_PEER_PLAINTEXT_ENCRYPTED));
	#endif
		
		if (peerInfo.flags & libt::peer_info::interesting)
			status_vec.push_back(app().res_wstr(HAL_PEER_INTERESTING));	
		if (peerInfo.flags & libt::peer_info::choked)
			status_vec.push_back(app().res_wstr(HAL_PEER_CHOKED));	
		if (peerInfo.flags & libt::peer_info::remote_interested)
			status_vec.push_back(app().res_wstr(HAL_PEER_REMOTE_INTERESTING));	
		if (peerInfo.flags & libt::peer_info::remote_choked)
			status_vec.push_back(app().res_wstr(HAL_PEER_REMOTE_CHOKED));	
		if (peerInfo.flags & libt::peer_info::supports_extensions)
			status_vec.push_back(app().res_wstr(HAL_PEER_SUPPORT_EXTENSIONS));	
	//	if (peerInfo.flags & libt::peer_info::local_connection)						// Not sure whats up here?
	//		status_vec.push_back(app().res_wstr(HAL_PEER_LOCAL_CONNECTION));			
		if (peerInfo.flags & libt::peer_info::queued)
			status_vec.push_back(app().res_wstr(HAL_PEER_QUEUED));
	}
	
	seed = (peerInfo.flags & libt::peer_info::seed) ? true : false;
	
	if (!status_vec.empty()) status = status_vec[0];
	
	if (status_vec.size() > 1)
	{
		for (size_t i=1; i<status_vec.size(); ++i)
		{
			status += L"; ";
			status += status_vec[i];
		}
	}	
}

const cache_details bit::get_cache_details() const
{
	return pimpl()->get_cache_details();
}

void bit::get_all_peer_details(const std::string& filename, PeerDetails& peerContainer)
{
	get_all_peer_details(from_utf8_safe(filename), peerContainer);
}

void bit::get_all_peer_details(const std::wstring& filename, PeerDetails& peerContainer)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->get_peer_details(peerContainer);
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "get_all_peer_details")
}

void bit::get_all_file_details(const std::string& filename, FileDetails& fileDetails)
{
	get_all_file_details(from_utf8_safe(filename), fileDetails);
}

void bit::get_all_file_details(const std::wstring& filename, FileDetails& fileDetails)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->get_file_details(fileDetails);
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "get_all_file_details")
}

bool bit::is_torrent(const std::string& filename)
{	
	return is_torrent(hal::to_wstr_shim(filename));
}

bool bit::is_torrent(const std::wstring& filename)
{	
	try {
	
	return pimpl()->the_torrents_.exists(filename);
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "isTorrent")
	
	return false;
}

void bit::pause_torrent(const std::string& filename)
{
	pause_torrent(hal::to_wstr_shim(filename));
}

void bit::pause_torrent(const std::wstring& filename)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->pause();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "pauseTorrent")
}

void bit::resume_torrent(const std::string& filename)
{
	resume_torrent(hal::to_wstr_shim(filename));
}

void bit::resume_torrent(const std::wstring& filename)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->resume();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "resumeTorrent")
}

void bit::stop_torrent(const std::string& filename)
{
	stop_torrent(hal::to_wstr_shim(filename));
}

void bit::stop_torrent(const std::wstring& filename)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->stop();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "stopTorrent")
}

bool bit::is_torrent_active(const std::string& filename)
{
	return is_torrent_active(hal::to_wstr_shim(filename));
}

bool bit::is_torrent_active(const std::wstring& filename)
{
	try {
	
	return pimpl()->the_torrents_.get(filename)->is_active();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "isTorrentActive")
	
	return false; // ??? is this correct
}

void bit::reannounce_torrent(const std::string& filename)
{
	reannounce_torrent(hal::to_wstr_shim(filename));
}

void bit::reannounce_torrent(const std::wstring& filename)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->handle().force_reannounce();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "reannounceTorrent")
}


void bit::recheck_torrent(const std::string& filename)
{
	recheck_torrent(hal::to_wstr_shim(filename));
}

void bit::recheck_torrent(const std::wstring& filename)
{
	try {
	
	pimpl()->the_torrents_.get(filename)->force_recheck();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(filename, "recheckTorrent")
}

void bit::remove_torrent_wstr(const std::wstring& filename)
{
	pimpl()->remove_torrent(filename);
}

void bit::remove_torrent_wipe_files_wstr(const std::wstring& filename)
{
	pimpl()->remove_torrent_wipe_files(hal::to_wstr_shim(filename));
}

void bit::pause_all_torrents()
{	
	try {
	
	for (TorrentManager::torrentByName::iterator i=pimpl()->the_torrents_.begin(), e=pimpl()->the_torrents_.end();
		i != e; ++i)
	{		
		if ((*i).torrent->in_session())
			(*i).torrent->pause();
	}
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH("Torrent Unknown!", "pauseAllTorrents")
}

void bit::unpause_all_torrents()
{	
	try {
	
	for (TorrentManager::torrentByName::iterator i=pimpl()->the_torrents_.begin(), e=pimpl()->the_torrents_.end();
		i != e; ++i)
	{
		if ((*i).torrent->in_session() && (*i).torrent->get_state() == torrent_details::torrent_paused)
			(*i).torrent->resume();
	}
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH("Torrent Unknown!", "unpauseAllTorrents")
}

bit::torrent::torrent()
{}

bit::torrent::torrent(boost::shared_ptr<torrent_internal> p) :
	ptr(p)
{}

bool bit::torrent::is_open() const
{
	return ptr;
}

bit::torrent::exec_around_ptr::proxy::proxy(torrent_internal* t) : 
	t_(t),
	l_(t->mutex_)
{
//	HAL_DEV_MSG(L"Ctor proxy");
}

bit::torrent::exec_around_ptr::proxy::~proxy() 
{
//	HAL_DEV_MSG(L"Dtor proxy");
}

const std::wstring bit::torrent::get_name() const
{
	try {
	
	return ptr->name();
	
	} HAL_GENERIC_TORRENT_EXCEPTION_CATCH(L"Torrent Unknown", "torrent::get_name()")
	
	return 0;
}

float bit::torrent::get_ratio() const
{
	try {
	
	return ptr->get_ratio();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_ratio")
	
	return 0;
}

void bit::torrent::set_ratio(float r)
{
	try {

	ptr->set_ratio(r);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_ratio")
}

std::pair<int, int> bit::torrent::get_connection_limits() const
{
	try {
	
	return ptr->get_connection_limit();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_connection_limits")
	
	return std::make_pair(-1, -1);
}

void bit::torrent::set_connection_limits(const std::pair<int, int>& l)
{
	try {
	
	ptr->set_connection_limit(l.first, l.second);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_connection_limits")
}

std::pair<float, float> bit::torrent::get_rate_limits() const
{
	try {
	
	return ptr->get_transfer_speed();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_rate_limits")
	
	return std::pair<float, float>(-1.0, -1.0);
}

void bit::torrent::set_rate_limits(const std::pair<float, float>& l)
{
	try {
	
	ptr->set_transfer_speed(l.first, l.second);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_rate_limits")
}

wpath bit::torrent::get_save_directory() const
{
	try {
	
	return ptr->get_save_directory();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_save_directory")
	
	return L"";
}

void bit::torrent::set_save_directory(const wpath& s)
{
	try {
	
	ptr->set_save_directory(s);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_save_directory")
}

wpath bit::torrent::get_move_to_directory() const
{
	try {
	
	return ptr->get_move_to_directory();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_save_directory")
	
	return L"";
}

void bit::torrent::set_move_to_directory(const wpath& m)
{
	try {
	
	ptr->set_move_to_directory(m);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_move_to_directory")
}

std::pair<wstring, wstring> bit::torrent::get_tracker_login() const
{
	try {
	
	return ptr->get_tracker_login();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("get_tracker_login")
	
	return std::make_pair(L"!!! exception thrown !!!", L"!!! exception thrown !!!");
}

void bit::torrent::set_tracker_login(const std::pair<wstring, wstring>& p)
{
	try {
	
	ptr->set_tracker_login(p.first, p.second);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_tracker_login")
}

bool bit::torrent::get_is_active() const
{
	try {
	
	return ptr->is_active();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_is_active")
	
	return L"";
}

bool bit::torrent::get_in_session() const
{
	try {
	
	return ptr->in_session();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_in_session")
	
	return L"";
}

std::vector<tracker_detail> bit::torrent::get_trackers() const
{
	try {
	
	return ptr->get_trackers();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_trackers")
	
	return std::vector<tracker_detail>();
}

void bit::torrent::set_trackers(const std::vector<tracker_detail>& trackers)
{
	try {
	
	ptr->set_trackers(trackers);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_trackers")
}

void bit::torrent::reset_trackers()
{
	try {
	
	ptr->reset_trackers();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_trackers")
}

void bit::torrent::set_file_priorities(const std::pair<std::vector<int>, int>& p)
{
	try { 

	ptr->set_file_priorities(p.first, p.second);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_trackers")
}

void bit::torrent::adjust_queue_position(bit::queue_adjustments adjust)
{
	try { 

	ptr->adjust_queue_position(adjust);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::adjust_queue_position")
}

bool bit::torrent::get_managed() const
{
	try {
	
	return ptr->is_managed();
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::get_managed")
	
	return false;
}

void bit::torrent::set_managed(bool m)
{
	try {
	
	ptr->set_managed(m);
	
	} HAL_GENERIC_TORRENT_PROP_EXCEPTION_CATCH("torrent::set_managed")
}

void bit::start_event_receiver()
{
	try {

	event_log.post(shared_ptr<EventDetail>(new EventMsg(L"Starting event handler.")));

	pimpl()->start_alert_handler();
	
	} HAL_GENERIC_PIMPL_EXCEPTION_CATCH("bit::start_event_receiver()")
}

void bit::stop_event_receiver()
{
	try {

	event_log.post(shared_ptr<EventDetail>(new EventMsg(L"Stopping event handler.")));

	pimpl()->stop_alert_handler();
	
	} HAL_GENERIC_PIMPL_EXCEPTION_CATCH("bit::stop_event_receiver()")
}

int bit::default_torrent_max_connections() { return pimpl()->default_torrent_max_connections_; }
int bit::default_torrent_max_uploads() { return pimpl()->default_torrent_max_uploads_; }
float bit::default_torrent_download() { return pimpl()->default_torrent_download_; }
float bit::default_torrent_upload() { return pimpl()->default_torrent_upload_; }
	
};