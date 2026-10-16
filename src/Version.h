#pragma once

#include <QString>

// Ce que c'est, et quand ça a été fait.
//
// La version est celle du fichier VERSION à la racine des sources -- trois
// nombres, avancés à la main quand quelque chose mérite d'être appelé une
// nouvelle version. Le build est le moment où le binaire a été compilé, en
// aammjjhhmmss : deux builds ne portent jamais le même, et c'est ce qui
// distingue le 1.0.0 d'hier de celui qui est devant vous.
//
// Les deux sont définis dans un fichier écrit à chaque build ; voir
// cmake/GenerateVersion.cmake.
extern const char *const kBelotesolVersion;
extern const char *const kBelotesolBuildVersion;

inline QString appVersion() { return QString::fromLatin1(kBelotesolVersion); }
inline QString buildVersion() { return QString::fromLatin1(kBelotesolBuildVersion); }

// Les deux ensemble, pour une ligne qui a la place des deux.
inline QString fullVersion() {
    return appVersion() + QStringLiteral(" (build ") + buildVersion() + QLatin1Char(')');
}
