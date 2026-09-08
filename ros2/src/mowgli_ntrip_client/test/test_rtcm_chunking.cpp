#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "mowgli_ntrip_client/ntrip_client_node.hpp"

namespace
{

TEST(RtcmChunking, PreservesRequiredBoundaryInputs)
{
  constexpr std::size_t kMaxRtcmMessageBytes = 720;
  const std::vector<std::size_t> sizes{1, 179, 180, 181, 719, 720, 721, 4096};

  for (const auto size : sizes) {
    std::vector<std::uint8_t> input;
    input.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
      input.push_back(static_cast<std::uint8_t>(index));
    }

    const auto chunks = mowgli_ntrip_client::NtripClientNode::split_rtcm_chunks(input);
    std::vector<std::uint8_t> output;
    for (const auto & chunk : chunks) {
      EXPECT_LE(chunk.size(), kMaxRtcmMessageBytes) << "input size=" << size;
      output.insert(output.end(), chunk.begin(), chunk.end());
    }
    EXPECT_EQ(output, input) << "input size=" << size;
  }
}

}  // namespace
