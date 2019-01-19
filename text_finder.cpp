#include "text_finder.h"
#include <QtDebug>
#include <QCryptographicHash>
#include <QDirIterator>


void text_finder::find_text(QString const& text) {
    alive = true;
    if (!alive) {
        return;
    }
    if (all_trigrams.empty()) {
        emit progress_changed(100);
        emit search_finished();
        return;
    }
    QVector<QString> trigrams;
    for (int i = 0; i + 2 < text.size(); i++) {
        trigrams.push_back(text.mid(i, 3));
    }
    qint64 cnt = 0;
    for (auto const& file_info : all_trigrams) {
        if (!alive) {
            break;
        }
        bool flag = true;
        for (auto const& trigram : trigrams) {
            if (!file_info.first.contains(trigram)) {
                flag = false;
                break;
            }
        }
        if (flag) {
            search_in_file(file_info.second, text);
        }
        emit progress_changed(100 * (++cnt) / all_trigrams.size());
    }
    emit search_finished();
}

void text_finder::index_directory(QString dir) {
    alive = true;
    all_trigrams.clear();
    QDirIterator it(dir, QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
    QFileInfoList all;
    while (it.hasNext()) {
        if (!alive) {
            break;
        }
        all.append(it.next());
    }
    if (all.empty()) {
        emit progress_changed(100);
    }
    qint64 cnt = 0;
    for (auto const& file_info : all) {
        if(!alive) {
            break;
        }
        add_file_info(file_info);
        emit progress_changed(100 * (++cnt) / all.size());
    }
    emit indexing_finished();
}

void text_finder::kill() {
    alive = false;
}

void text_finder::add_file_info(QFileInfo const& file_info) {
    if (file_info.size() < 5) {
        return;
    }
    QSet<QString> trigrams;
    QFile file(file_info.filePath());
    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        QString trigram;
        for (int i = 0; i < 3; i++) {
            QChar tmp;
            in >> tmp;
            if (!tmp.isPrint() && !tmp.isSpace()) {
                return;
            }
            trigram += tmp;
        }
        trigrams.insert(trigram);
        while (!in.atEnd()) {
            if (!alive) {
                break;
            }
            for (int i = 0; i < 2; i++) {
                trigram[i] = trigram[i + 1];
            }
            QChar tmp;
            in >> tmp;
            if (!tmp.isPrint() && !tmp.isSpace()) {
                return;
            }
            trigram[2] = tmp;
            trigrams.insert(trigram);
        }
    }
    if (trigrams.size() < 50000) {
        all_trigrams.push_back(qMakePair(std::move(trigrams), file_info));
    }
}

void text_finder::search_in_file(const QFileInfo &file_info, const QString &text) {
    QVector<int> prefix_function(text.size());
    for (int i = 1; i < text.size(); i++) {
        int j = prefix_function[i - 1];
        while (j > 0 && text.at(i) != text.at(j)) {
            j = prefix_function[j - 1];
        }
        if (text.at(i) == text.at(j)) {
            j++;
        }
        prefix_function[i] = j;
    }
    prefix_function.append(0);
    int prev = 0;
    QFile file(file_info.filePath());
    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QChar tmp;
            in >> tmp;
            int j = prev;
            while (j > 0 && (j == text.size() || tmp != text.at(j))) {
                j = prefix_function[j - 1];
            }
            if (j < text.size() && tmp == text.at(j)) {
                j++;
            }
            if (j == text.size()) {
                emit occurrence_finded(file_info.filePath(), text);
            }
            prev = j;
        }
    }
    emit search_finished();
}
