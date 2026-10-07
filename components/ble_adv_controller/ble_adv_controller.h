#pragma once

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "ble_adv_handler.h"
#include <vector>
#include <list>

namespace esphome {
namespace bleadvcontroller {


//  Base class to define a dynamic Configuration
template < class BaseEntity >
class BleAdvDynConfig: public BaseEntity
{
public:
  void init(const char * name, const StringRef & parent_name) {
  // Name must stay valid -> keep the built string as a member
  this->ref_name_ = std::string(parent_name) + " - " + std::string(name);

  // name, object_id_hash, packed flags (entity_category in bits 26-27)
  this->configure_entity_(
      this->ref_name_.c_str(),
      fnv1_hash(this->ref_name_),
      static_cast<uint32_t>(EntityCategory::ENTITY_CATEGORY_CONFIG) << 26);

  // sub_init() must restore the value AND call publish_state() itself
  this->sub_init();
}
 /** moje zmena 2026 - 
   void init(const char * name, const StringRef & parent_name) {
    // Due to the use of sh... StringRef, we are forced to keep a ref on the built string...
    this->ref_name_ = std::string(parent_name) + " - " + std::string(name);
    this->set_object_id(this->ref_name_.c_str());
    this->set_name(this->ref_name_.c_str());
    this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
    this->sub_init();
    this->publish_state(this->state);
  } */

  // register to App and restore from config / saved data
  virtual void sub_init() = 0;

protected:
  std::string ref_name_;
  ESPPreferenceObject rtc_{nullptr};
};

/**
  BleAdvSelect: basic implementation of 'Select' to handle configuration choice from HA directly
 */
 /** moje zmena 2026 
class BleAdvSelect: public BleAdvDynConfig < select::Select > {
protected:
  void control(const std::string &value) override;
  void sub_init() override;
};
 */

class BleAdvSelect: public BleAdvDynConfig < select::Select > {
public:
  void set_options(const std::vector<std::string> &options);
protected:
  void control(const std::string &value) override;
  void sub_init() override;

  std::vector<std::string> option_strs_;   // owns the strings
  FixedVector<const char *> option_ptrs_;  // pointers into option_strs_
};

/**
  BleAdvNumber: basic implementation of 'Number' to handle duration(s) choice from HA directly
 */
class BleAdvNumber: public BleAdvDynConfig < number::Number > {
protected:
  void control(float value) override;
  void sub_init() override;
};

/**
  BleAdvController:
    One physical device controlled == One Controller.
    Referenced by Entities as their parent to perform commands.
    Chooses which encoder(s) to be used to issue a command
    Interacts with the BleAdvHandler for Queue processing
 */
class BleAdvController : public Component, public EntityBase
#ifdef USE_API
  , public api::CustomAPIDevice
#endif
{
public:
  void setup() override;
  void loop() override;
  virtual void dump_config() override;
  
  void set_min_tx_duration(int tx_duration, int min, int max, int step);
  uint32_t get_min_tx_duration() { return (uint32_t)this->number_duration_.state; }
  void set_max_tx_duration(uint32_t tx_duration) { this->max_tx_duration_ = tx_duration; }
  void set_seq_duration(uint32_t seq_duration) { this->seq_duration_ = seq_duration; }
  void set_forced_id(uint32_t forced_id) { this->params_.id_ = forced_id; }
  void set_forced_id(const std::string & str_id) { this->params_.id_ = fnv1_hash(str_id); }
  void set_index(uint8_t index) { this->params_.index_ = index; }
  void set_encoding_and_variant(const std::string & encoding, const std::string & variant);
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  bool is_reversed() const { return this->reversed_; }
  bool is_supported(const Command &cmd) { return this->cur_encoder_->is_supported(cmd); }
  void set_show_config(bool show_config) { this->show_config_ = show_config; }
  bool is_show_config() { return this->show_config_; }

  void set_handler(BleAdvHandler * handler) { this->handler_ = handler; }
// moje zmena 2026 - void refresh_encoder(std::string id, size_t index);
  void refresh_encoder(size_t index);

#ifdef USE_API
  // Services
  void on_pair();
  void on_unpair();
  void on_cmd(float cmd, float arg0, float arg1, float arg2, float arg3);
  void on_raw_inject(std::string raw);
#endif

  bool enqueue(Command &cmd);

protected:

  uint32_t max_tx_duration_ = 3000;
  uint32_t seq_duration_ = 150;

  ControllerParam_t params_;

  bool reversed_;

  bool show_config_{false};
  BleAdvSelect select_encoding_;
  BleAdvEncoder * cur_encoder_{nullptr};
  BleAdvNumber number_duration_;
  BleAdvHandler * handler_{nullptr};

  class QueueItem {
  public:
    QueueItem(CommandType cmd_type): cmd_type_(cmd_type) {}
    CommandType cmd_type_;
    std::vector< BleAdvParam > params_;
  
    // Only move operators to avoid data copy
    QueueItem(QueueItem&&) = default;
    QueueItem& operator=(QueueItem&&) = default;
  };
  std::list< QueueItem > commands_;

  // Being advertised data properties
  uint32_t adv_start_time_ = 0;
  uint16_t adv_id_ = 0;
};

/**
  BleAdvEntity: 
    Base class for implementation of Entities, referencing the parent BleAdvController
 */
class BleAdvEntity: public Component, public Parented < BleAdvController >
{
  public:
    virtual void dump_config() override = 0;

  protected:
    void dump_config_base(const char * tag);
    void command(CommandType cmd, const std::vector<uint8_t> &args);
    void command(CommandType cmd, uint8_t value1 = 0, uint8_t value2 = 0);
};

} //namespace bleadvcontroller
} //namespace esphome
