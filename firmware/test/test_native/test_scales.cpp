#include <gtest/gtest.h>
// uncomment line below if you plan to use GMock
// #include <gmock/gmock.h>
#include <scales.cpp>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // if you plan to use GMock, replace the line above with
  // ::testing::InitGoogleMock(&argc, argv);

  if (RUN_ALL_TESTS())
    ;

  // Always return zero-code and allow PlatformIO to parse results
  return 0;
}

// C  C# D  D# E  F  F# G  G# A  A# B
// 0  1  2  3  4  5  6  7  8  9  10 11
//

// Test Chromatic
TEST(buildScale, Chromatic)
{
  bool expected[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  bool result[12];
  buildScale(0, 0, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test Major C
TEST(buildScale, Major_C)
{
  bool expected[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
  bool result[12];
  buildScale(1, 0, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test Major C#
TEST(buildScale, Major_Csharp)
{
  // Notes: C♯, D♯, E♯, F♯, G♯, A♯, B♯, C♯
  bool expected[12] = {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0};
  bool result[12];
  buildScale(1, 1, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test Major D#
TEST(buildScale, Major_Dsharp)
{
  // Notes: D♯, E♯, F, G♯, A♯, B♯, C, D♯
  bool expected[12] = {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0};
  bool result[12];
  buildScale(1, 3, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test A Minor
TEST(buildScale, Minor_A)
{
  // Notes: A, B, C, D, E, F, G, A
  bool expected[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
  bool result[12];
  buildScale(2, 9, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test A# Minor
TEST(buildScale, Minor_Asharp)
{ // Notes: A#, C, C#, D#, F, F#, G#, A#
  bool expected[12] = {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0};
  bool result[12];
  buildScale(2, 10, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

TEST(buildScale, Major_D)
{
  // Notes: D, E, F♯, G, A, B, C♯, D
  bool expected[12] = {0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1};
  bool result[12];
  buildScale(1, 2, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test C Minor
TEST(buildScale, Minor_C)
{
  // Notes: C, D, Eb, F, G, Ab, Bb, C
  bool expected[12] = {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0};
  bool result[12];
  buildScale(2, 0, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

TEST(buildScale, Minor_F)
{
  // Notes: F, G, Ab, Bb, C, Db, Eb, F
  bool expected[12] = {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0};
  bool result[12];
  buildScale(2, 5, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}

// Test Pentatonic Minor for G
TEST(buildScale, PentatonicMinor_G)
{
  // Notes: G, B♭, C, D, F, G
  bool expected[12] = {1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0};
  bool result[12];
  buildScale(8, 7, result);
  for (int i = 0; i < 12; i++)
  {
    EXPECT_EQ(expected[i], result[i]);
  }
}
