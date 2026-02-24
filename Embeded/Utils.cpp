#include "Utils.hpp"

std::atomic_uint pwmChannelCounter(0);

String textPlain(F("text/plain"));
String textHtml(F("text/html"));
String applicationJson(F("application/json"));
String EMPTY_STR(F(""));

size_t MemoryBufferHolder::sizeInt() noexcept {
  return buffer->pool->getElementSize();
}

void MemoryBufferHolder::release() noexcept {
  buffer->pool->release(buffer);
}
