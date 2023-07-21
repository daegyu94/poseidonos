// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: prefetch.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_prefetch_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_prefetch_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3015000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3015008 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_prefetch_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_prefetch_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[3]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_prefetch_2eproto;
::PROTOBUF_NAMESPACE_ID::Metadata descriptor_table_prefetch_2eproto_metadata_getter(int index);
namespace prefetch {
class PrefetchMsg;
struct PrefetchMsgDefaultTypeInternal;
extern PrefetchMsgDefaultTypeInternal _PrefetchMsg_default_instance_;
class PrefetchReply;
struct PrefetchReplyDefaultTypeInternal;
extern PrefetchReplyDefaultTypeInternal _PrefetchReply_default_instance_;
class PrefetchRequest;
struct PrefetchRequestDefaultTypeInternal;
extern PrefetchRequestDefaultTypeInternal _PrefetchRequest_default_instance_;
}  // namespace prefetch
PROTOBUF_NAMESPACE_OPEN
template<> ::prefetch::PrefetchMsg* Arena::CreateMaybeMessage<::prefetch::PrefetchMsg>(Arena*);
template<> ::prefetch::PrefetchReply* Arena::CreateMaybeMessage<::prefetch::PrefetchReply>(Arena*);
template<> ::prefetch::PrefetchRequest* Arena::CreateMaybeMessage<::prefetch::PrefetchRequest>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace prefetch {

// ===================================================================

class PrefetchMsg PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:prefetch.PrefetchMsg) */ {
 public:
  inline PrefetchMsg() : PrefetchMsg(nullptr) {}
  virtual ~PrefetchMsg();
  explicit constexpr PrefetchMsg(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  PrefetchMsg(const PrefetchMsg& from);
  PrefetchMsg(PrefetchMsg&& from) noexcept
    : PrefetchMsg() {
    *this = ::std::move(from);
  }

  inline PrefetchMsg& operator=(const PrefetchMsg& from) {
    CopyFrom(from);
    return *this;
  }
  inline PrefetchMsg& operator=(PrefetchMsg&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const PrefetchMsg& default_instance() {
    return *internal_default_instance();
  }
  static inline const PrefetchMsg* internal_default_instance() {
    return reinterpret_cast<const PrefetchMsg*>(
               &_PrefetchMsg_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(PrefetchMsg& a, PrefetchMsg& b) {
    a.Swap(&b);
  }
  inline void Swap(PrefetchMsg* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(PrefetchMsg* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline PrefetchMsg* New() const final {
    return CreateMaybeMessage<PrefetchMsg>(nullptr);
  }

  PrefetchMsg* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<PrefetchMsg>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const PrefetchMsg& from);
  void MergeFrom(const PrefetchMsg& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(PrefetchMsg* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "prefetch.PrefetchMsg";
  }
  protected:
  explicit PrefetchMsg(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    return ::descriptor_table_prefetch_2eproto_metadata_getter(kIndexInFileMessages);
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kSubsysIdFieldNumber = 1,
    kNsIdFieldNumber = 2,
    kPbaFieldNumber = 3,
  };
  // uint32 subsys_id = 1;
  void clear_subsys_id();
  ::PROTOBUF_NAMESPACE_ID::uint32 subsys_id() const;
  void set_subsys_id(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_subsys_id() const;
  void _internal_set_subsys_id(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 ns_id = 2;
  void clear_ns_id();
  ::PROTOBUF_NAMESPACE_ID::uint32 ns_id() const;
  void set_ns_id(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_ns_id() const;
  void _internal_set_ns_id(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint64 pba = 3;
  void clear_pba();
  ::PROTOBUF_NAMESPACE_ID::uint64 pba() const;
  void set_pba(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_pba() const;
  void _internal_set_pba(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // @@protoc_insertion_point(class_scope:prefetch.PrefetchMsg)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 subsys_id_;
  ::PROTOBUF_NAMESPACE_ID::uint32 ns_id_;
  ::PROTOBUF_NAMESPACE_ID::uint64 pba_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_prefetch_2eproto;
};
// -------------------------------------------------------------------

class PrefetchRequest PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:prefetch.PrefetchRequest) */ {
 public:
  inline PrefetchRequest() : PrefetchRequest(nullptr) {}
  virtual ~PrefetchRequest();
  explicit constexpr PrefetchRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  PrefetchRequest(const PrefetchRequest& from);
  PrefetchRequest(PrefetchRequest&& from) noexcept
    : PrefetchRequest() {
    *this = ::std::move(from);
  }

  inline PrefetchRequest& operator=(const PrefetchRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline PrefetchRequest& operator=(PrefetchRequest&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const PrefetchRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const PrefetchRequest* internal_default_instance() {
    return reinterpret_cast<const PrefetchRequest*>(
               &_PrefetchRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(PrefetchRequest& a, PrefetchRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(PrefetchRequest* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(PrefetchRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline PrefetchRequest* New() const final {
    return CreateMaybeMessage<PrefetchRequest>(nullptr);
  }

  PrefetchRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<PrefetchRequest>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const PrefetchRequest& from);
  void MergeFrom(const PrefetchRequest& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(PrefetchRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "prefetch.PrefetchRequest";
  }
  protected:
  explicit PrefetchRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    return ::descriptor_table_prefetch_2eproto_metadata_getter(kIndexInFileMessages);
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kMsgsFieldNumber = 1,
  };
  // repeated .prefetch.PrefetchMsg msgs = 1;
  int msgs_size() const;
  private:
  int _internal_msgs_size() const;
  public:
  void clear_msgs();
  ::prefetch::PrefetchMsg* mutable_msgs(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::prefetch::PrefetchMsg >*
      mutable_msgs();
  private:
  const ::prefetch::PrefetchMsg& _internal_msgs(int index) const;
  ::prefetch::PrefetchMsg* _internal_add_msgs();
  public:
  const ::prefetch::PrefetchMsg& msgs(int index) const;
  ::prefetch::PrefetchMsg* add_msgs();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::prefetch::PrefetchMsg >&
      msgs() const;

  // @@protoc_insertion_point(class_scope:prefetch.PrefetchRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::prefetch::PrefetchMsg > msgs_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_prefetch_2eproto;
};
// -------------------------------------------------------------------

class PrefetchReply PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:prefetch.PrefetchReply) */ {
 public:
  inline PrefetchReply() : PrefetchReply(nullptr) {}
  virtual ~PrefetchReply();
  explicit constexpr PrefetchReply(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  PrefetchReply(const PrefetchReply& from);
  PrefetchReply(PrefetchReply&& from) noexcept
    : PrefetchReply() {
    *this = ::std::move(from);
  }

  inline PrefetchReply& operator=(const PrefetchReply& from) {
    CopyFrom(from);
    return *this;
  }
  inline PrefetchReply& operator=(PrefetchReply&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const PrefetchReply& default_instance() {
    return *internal_default_instance();
  }
  static inline const PrefetchReply* internal_default_instance() {
    return reinterpret_cast<const PrefetchReply*>(
               &_PrefetchReply_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(PrefetchReply& a, PrefetchReply& b) {
    a.Swap(&b);
  }
  inline void Swap(PrefetchReply* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(PrefetchReply* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline PrefetchReply* New() const final {
    return CreateMaybeMessage<PrefetchReply>(nullptr);
  }

  PrefetchReply* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<PrefetchReply>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const PrefetchReply& from);
  void MergeFrom(const PrefetchReply& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(PrefetchReply* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "prefetch.PrefetchReply";
  }
  protected:
  explicit PrefetchReply(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    return ::descriptor_table_prefetch_2eproto_metadata_getter(kIndexInFileMessages);
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kSuccessFieldNumber = 1,
  };
  // bool success = 1;
  void clear_success();
  bool success() const;
  void set_success(bool value);
  private:
  bool _internal_success() const;
  void _internal_set_success(bool value);
  public:

  // @@protoc_insertion_point(class_scope:prefetch.PrefetchReply)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  bool success_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_prefetch_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// PrefetchMsg

// uint32 subsys_id = 1;
inline void PrefetchMsg::clear_subsys_id() {
  subsys_id_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 PrefetchMsg::_internal_subsys_id() const {
  return subsys_id_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 PrefetchMsg::subsys_id() const {
  // @@protoc_insertion_point(field_get:prefetch.PrefetchMsg.subsys_id)
  return _internal_subsys_id();
}
inline void PrefetchMsg::_internal_set_subsys_id(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  subsys_id_ = value;
}
inline void PrefetchMsg::set_subsys_id(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_subsys_id(value);
  // @@protoc_insertion_point(field_set:prefetch.PrefetchMsg.subsys_id)
}

// uint32 ns_id = 2;
inline void PrefetchMsg::clear_ns_id() {
  ns_id_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 PrefetchMsg::_internal_ns_id() const {
  return ns_id_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 PrefetchMsg::ns_id() const {
  // @@protoc_insertion_point(field_get:prefetch.PrefetchMsg.ns_id)
  return _internal_ns_id();
}
inline void PrefetchMsg::_internal_set_ns_id(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  ns_id_ = value;
}
inline void PrefetchMsg::set_ns_id(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_ns_id(value);
  // @@protoc_insertion_point(field_set:prefetch.PrefetchMsg.ns_id)
}

// uint64 pba = 3;
inline void PrefetchMsg::clear_pba() {
  pba_ = PROTOBUF_ULONGLONG(0);
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 PrefetchMsg::_internal_pba() const {
  return pba_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 PrefetchMsg::pba() const {
  // @@protoc_insertion_point(field_get:prefetch.PrefetchMsg.pba)
  return _internal_pba();
}
inline void PrefetchMsg::_internal_set_pba(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  pba_ = value;
}
inline void PrefetchMsg::set_pba(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_pba(value);
  // @@protoc_insertion_point(field_set:prefetch.PrefetchMsg.pba)
}

// -------------------------------------------------------------------

// PrefetchRequest

// repeated .prefetch.PrefetchMsg msgs = 1;
inline int PrefetchRequest::_internal_msgs_size() const {
  return msgs_.size();
}
inline int PrefetchRequest::msgs_size() const {
  return _internal_msgs_size();
}
inline void PrefetchRequest::clear_msgs() {
  msgs_.Clear();
}
inline ::prefetch::PrefetchMsg* PrefetchRequest::mutable_msgs(int index) {
  // @@protoc_insertion_point(field_mutable:prefetch.PrefetchRequest.msgs)
  return msgs_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::prefetch::PrefetchMsg >*
PrefetchRequest::mutable_msgs() {
  // @@protoc_insertion_point(field_mutable_list:prefetch.PrefetchRequest.msgs)
  return &msgs_;
}
inline const ::prefetch::PrefetchMsg& PrefetchRequest::_internal_msgs(int index) const {
  return msgs_.Get(index);
}
inline const ::prefetch::PrefetchMsg& PrefetchRequest::msgs(int index) const {
  // @@protoc_insertion_point(field_get:prefetch.PrefetchRequest.msgs)
  return _internal_msgs(index);
}
inline ::prefetch::PrefetchMsg* PrefetchRequest::_internal_add_msgs() {
  return msgs_.Add();
}
inline ::prefetch::PrefetchMsg* PrefetchRequest::add_msgs() {
  // @@protoc_insertion_point(field_add:prefetch.PrefetchRequest.msgs)
  return _internal_add_msgs();
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::prefetch::PrefetchMsg >&
PrefetchRequest::msgs() const {
  // @@protoc_insertion_point(field_list:prefetch.PrefetchRequest.msgs)
  return msgs_;
}

// -------------------------------------------------------------------

// PrefetchReply

// bool success = 1;
inline void PrefetchReply::clear_success() {
  success_ = false;
}
inline bool PrefetchReply::_internal_success() const {
  return success_;
}
inline bool PrefetchReply::success() const {
  // @@protoc_insertion_point(field_get:prefetch.PrefetchReply.success)
  return _internal_success();
}
inline void PrefetchReply::_internal_set_success(bool value) {
  
  success_ = value;
}
inline void PrefetchReply::set_success(bool value) {
  _internal_set_success(value);
  // @@protoc_insertion_point(field_set:prefetch.PrefetchReply.success)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace prefetch

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_prefetch_2eproto
