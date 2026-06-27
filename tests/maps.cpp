#include <gtest/gtest.h>
#include "helpers.hpp"
#include <rmf/maps.hpp>

TEST(map, DefaultConstruction)
{
    rmf::Map map;
    EXPECT_FALSE(map.valid());
}
