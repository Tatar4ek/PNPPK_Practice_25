#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <QString>

namespace ConfigUtils {
    inline QString cleanKey(const QString &key) {
        return key.trimmed();
    }
}

#endif // CONFIG_UTILS_H
