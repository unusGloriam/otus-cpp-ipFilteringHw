#include "version_api.h"
#include "version.h"

int VersionAPI::getVersion() {
    return PROJECT_VERSION_PATCH;
}
