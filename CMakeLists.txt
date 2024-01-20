idf_component_register(
  SRCS 
  "src/BasicMetaInfoSchema.cpp"
  "src/BasicTopicInformation.cpp"
  "src/JsonDecoder.cpp"
  "src/JsonEncoder.cpp"
  "src/RawStream.cpp"
  "src/Registry.cpp"
  "src/SerializationType.cpp"
  "src/Stream.cpp"
  "src/StreamType.cpp"
  "src/TopicMessenger.cpp"
  "src/TopicTranslator.cpp"

  INCLUDE_DIRS
  "include"

  REQUIRES
  json
  )
