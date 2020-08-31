#include <dynamic_robot_localization/common/cumulative_static_transform_broadcaster.h>

namespace dynamic_robot_localization {
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  <CumulativeStaticTransformBroadcaster>  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	std::shared_ptr< CumulativeStaticTransformBroadcaster > CumulativeStaticTransformBroadcaster::singleton_ = std::shared_ptr< CumulativeStaticTransformBroadcaster >();
	std::shared_ptr< std::mutex > CumulativeStaticTransformBroadcaster::singleton__mutex_ = std::shared_ptr< std::mutex >(new std::mutex());

	std::shared_ptr< CumulativeStaticTransformBroadcaster > CumulativeStaticTransformBroadcaster::getSingleton(ros::NodeHandlePtr& node_handle) {
		if (!singleton_) {
			singleton__mutex_->lock();
			singleton_ = std::shared_ptr< CumulativeStaticTransformBroadcaster >(new CumulativeStaticTransformBroadcaster());
			singleton_->setup(node_handle);
			singleton__mutex_->unlock();
		}
		return singleton_;
	}


	void CumulativeStaticTransformBroadcaster::setup(ros::NodeHandlePtr& node_handle) {
		cached_static_tfs_mutex_ = std::shared_ptr< std::mutex >(new std::mutex());
		static_tf_subscriber_ = node_handle->subscribe("/tf_static", 10, &CumulativeStaticTransformBroadcaster::updateTFCache, this);
		static_tf_publisher_ = node_handle->advertise<tf2_msgs::TFMessage>("/tf_static", 10, true);
	}


	void CumulativeStaticTransformBroadcaster::updateTFCache(const tf2_msgs::TFMessageConstPtr& tf_msg) {
		updateTFCache(tf_msg->transforms);
	}


	void CumulativeStaticTransformBroadcaster::updateTFCache(const std::vector<geometry_msgs::TransformStamped>& tfs) {
		cached_static_tfs_mutex_->lock();
		for (size_t i = 0; i < tfs.size(); ++i) {
			std::stringstream key;
			key << tfs[i].header.frame_id << "__" << tfs[i].child_frame_id;

			std::map< std::string, geometry_msgs::TransformStamped >::iterator it = cached_static_tfs_.find(key.str());
			if (it != cached_static_tfs_.end()) {
				if (it->second.header.stamp.toNSec() < tfs[i].header.stamp.toNSec()) {
					cached_static_tfs_[key.str()] = tfs[i]; // update
				}
			} else {
				cached_static_tfs_[key.str()] = tfs[i]; // insert
			}
		}
		cached_static_tfs_mutex_->unlock();
	}


	void CumulativeStaticTransformBroadcaster::sendTransform(const geometry_msgs::TransformStamped& tf) {
		std::vector<geometry_msgs::TransformStamped> vector_tf;
		vector_tf.push_back(tf);
		sendTransform(vector_tf);
	}


	void CumulativeStaticTransformBroadcaster::sendTransform(const std::vector<geometry_msgs::TransformStamped>& tfs) {
		updateTFCache(tfs);
		sendCachedTFs();
	}


	void CumulativeStaticTransformBroadcaster::sendCachedTFs() {
		cached_static_tfs_mutex_->lock();
		tf2_msgs::TFMessagePtr tf_msg = tf2_msgs::TFMessagePtr(new tf2_msgs::TFMessage());
		for (std::map< std::string, geometry_msgs::TransformStamped >::iterator it = cached_static_tfs_.begin(); it != cached_static_tfs_.end(); ++it) {
			tf_msg->transforms.push_back(it->second);
		}
		static_tf_publisher_.publish(tf_msg);
		cached_static_tfs_mutex_->unlock();
	}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  </CumulativeStaticTransformBroadcaster>  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
} /* namespace dynamic_robot_localization */

