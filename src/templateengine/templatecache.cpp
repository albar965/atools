#include "templatecache.h"
#include <QDateTime>
#include <QStringList>
#include <QSet>
#include <QVariant>

using namespace stefanfrings;

TemplateCache::TemplateCache(const QHash<QString, QVariant>& settings, QObject *parent)
  : TemplateLoader(settings, parent)
{
  cache.setMaxCost(settings.value("cacheSize", "1000000").toInt());
  cacheTimeout = settings.value("cacheTime", "60000").toInt();
  qDebug("TemplateCache: timeout=%i, size=%i", cacheTimeout, cache.maxCost());
}

QString TemplateCache::tryFile(const QString localizedName)
{
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  mutex.lock();
  // search in cache
  qDebug("TemplateCache: trying cached %s", qPrintable(localizedName));
  CacheEntry *entry = cache.object(localizedName);
  if(entry && (cacheTimeout == 0 || entry->created > now - cacheTimeout))
  {
    mutex.unlock();
    return entry->document;
  }
  // search on filesystem
  entry = new CacheEntry();
  entry->created = now;
  entry->document = TemplateLoader::tryFile(localizedName);
  // Store in cache even when the file did not exist, to remember that there is no such file
  cache.insert(localizedName, entry, entry->document.size());
  mutex.unlock();
  return entry->document;
}
