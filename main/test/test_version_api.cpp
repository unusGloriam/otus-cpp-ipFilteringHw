#include "version_api/version_api.h"
#include <gtest/gtest.h>


TEST(VersionApi, GetVersion_TryToGetPrjVersion_SuccessCheck)
{
    ASSERT_TRUE(VersionAPI::getVersion()>0);
}
