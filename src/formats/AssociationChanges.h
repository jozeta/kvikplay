#pragma once

#include <QStringList>
#include <algorithm>

namespace kvikplay {

// What the File Associations dialog must do when saved.
struct AssociationChanges {
    QStringList toRegister;   // every ticked extension (registering is idempotent)
    QStringList toUnregister; // registered before, unticked now
};

inline AssociationChanges diffAssociations(const QStringList& previous, const QStringList& selected) {
    AssociationChanges ch;
    ch.toRegister = selected;
    for (const QString& ext : previous) {
        if (!selected.contains(ext)) ch.toUnregister.append(ext);
    }
    std::sort(ch.toRegister.begin(), ch.toRegister.end());
    std::sort(ch.toUnregister.begin(), ch.toUnregister.end());
    return ch;
}

} // namespace kvikplay
