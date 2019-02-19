// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: storage-message.proto

#ifndef PROTOBUF_INCLUDED_storage_2dmessage_2eproto
#define PROTOBUF_INCLUDED_storage_2dmessage_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006001
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_storage_2dmessage_2eproto 

namespace protobuf_storage_2dmessage_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[1];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_storage_2dmessage_2eproto
namespace proto {
class StorageMessage;
class StorageMessageDefaultTypeInternal;
extern StorageMessageDefaultTypeInternal _StorageMessage_default_instance_;
}  // namespace proto
namespace google {
namespace protobuf {
template<> ::proto::StorageMessage* Arena::CreateMaybeMessage<::proto::StorageMessage>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace proto {

// ===================================================================

class StorageMessage : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:proto.StorageMessage) */ {
 public:
  StorageMessage();
  virtual ~StorageMessage();

  StorageMessage(const StorageMessage& from);

  inline StorageMessage& operator=(const StorageMessage& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  StorageMessage(StorageMessage&& from) noexcept
    : StorageMessage() {
    *this = ::std::move(from);
  }

  inline StorageMessage& operator=(StorageMessage&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const StorageMessage& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const StorageMessage* internal_default_instance() {
    return reinterpret_cast<const StorageMessage*>(
               &_StorageMessage_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(StorageMessage* other);
  friend void swap(StorageMessage& a, StorageMessage& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline StorageMessage* New() const final {
    return CreateMaybeMessage<StorageMessage>(NULL);
  }

  StorageMessage* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<StorageMessage>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const StorageMessage& from);
  void MergeFrom(const StorageMessage& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(StorageMessage* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required string voted_for = 2;
  bool has_voted_for() const;
  void clear_voted_for();
  static const int kVotedForFieldNumber = 2;
  const ::std::string& voted_for() const;
  void set_voted_for(const ::std::string& value);
  #if LANG_CXX11
  void set_voted_for(::std::string&& value);
  #endif
  void set_voted_for(const char* value);
  void set_voted_for(const char* value, size_t size);
  ::std::string* mutable_voted_for();
  ::std::string* release_voted_for();
  void set_allocated_voted_for(::std::string* voted_for);

  // required int32 current_term = 1;
  bool has_current_term() const;
  void clear_current_term();
  static const int kCurrentTermFieldNumber = 1;
  ::google::protobuf::int32 current_term() const;
  void set_current_term(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:proto.StorageMessage)
 private:
  void set_has_current_term();
  void clear_has_current_term();
  void set_has_voted_for();
  void clear_has_voted_for();

  // helper for ByteSizeLong()
  size_t RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::HasBits<1> _has_bits_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr voted_for_;
  ::google::protobuf::int32 current_term_;
  friend struct ::protobuf_storage_2dmessage_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// StorageMessage

// required int32 current_term = 1;
inline bool StorageMessage::has_current_term() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void StorageMessage::set_has_current_term() {
  _has_bits_[0] |= 0x00000002u;
}
inline void StorageMessage::clear_has_current_term() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void StorageMessage::clear_current_term() {
  current_term_ = 0;
  clear_has_current_term();
}
inline ::google::protobuf::int32 StorageMessage::current_term() const {
  // @@protoc_insertion_point(field_get:proto.StorageMessage.current_term)
  return current_term_;
}
inline void StorageMessage::set_current_term(::google::protobuf::int32 value) {
  set_has_current_term();
  current_term_ = value;
  // @@protoc_insertion_point(field_set:proto.StorageMessage.current_term)
}

// required string voted_for = 2;
inline bool StorageMessage::has_voted_for() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void StorageMessage::set_has_voted_for() {
  _has_bits_[0] |= 0x00000001u;
}
inline void StorageMessage::clear_has_voted_for() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void StorageMessage::clear_voted_for() {
  voted_for_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_voted_for();
}
inline const ::std::string& StorageMessage::voted_for() const {
  // @@protoc_insertion_point(field_get:proto.StorageMessage.voted_for)
  return voted_for_.GetNoArena();
}
inline void StorageMessage::set_voted_for(const ::std::string& value) {
  set_has_voted_for();
  voted_for_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:proto.StorageMessage.voted_for)
}
#if LANG_CXX11
inline void StorageMessage::set_voted_for(::std::string&& value) {
  set_has_voted_for();
  voted_for_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:proto.StorageMessage.voted_for)
}
#endif
inline void StorageMessage::set_voted_for(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  set_has_voted_for();
  voted_for_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:proto.StorageMessage.voted_for)
}
inline void StorageMessage::set_voted_for(const char* value, size_t size) {
  set_has_voted_for();
  voted_for_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:proto.StorageMessage.voted_for)
}
inline ::std::string* StorageMessage::mutable_voted_for() {
  set_has_voted_for();
  // @@protoc_insertion_point(field_mutable:proto.StorageMessage.voted_for)
  return voted_for_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* StorageMessage::release_voted_for() {
  // @@protoc_insertion_point(field_release:proto.StorageMessage.voted_for)
  if (!has_voted_for()) {
    return NULL;
  }
  clear_has_voted_for();
  return voted_for_.ReleaseNonDefaultNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void StorageMessage::set_allocated_voted_for(::std::string* voted_for) {
  if (voted_for != NULL) {
    set_has_voted_for();
  } else {
    clear_has_voted_for();
  }
  voted_for_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), voted_for);
  // @@protoc_insertion_point(field_set_allocated:proto.StorageMessage.voted_for)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_storage_2dmessage_2eproto
