// ============================================================================
// Name        : BglReader.cpp
// Author      : Alexander Barthel
// Version     :
// Copyright   :
// Description : Hello World in C++, Ansi-style
// ============================================================================

#include "scenery/scenerycfg.h"
#include "sql/sqldatabase.h"
#include "sql/sqlexception.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "writer/datawriter.h"
#include "writer/routeresolver.h"
#include "bglreaderoptions.h"
#include <QString>
int main(int argc, char **argv)
{
  Q_UNUSED(argc);
  Q_UNUSED(argv);
  using atools::fs::scenery::SceneryCfg;
  using atools::fs::scenery::AreaVectorType;
  using atools::fs::scenery::AreaVectorIterType;
  using atools::fs::scenery::SceneryArea;

  using atools::fs::writer::RouteResolver;

  int retval = 0;
#if 0

  qInfo() << "Starting ...";
  try
  {

    BglReaderOptions options;
    context.set_main_group(options);
    context.parse(argc, argv);

    log.infoStream() << options;

    SceneryCfg cfg;
    cfg.read(options.getSceneryFile());

    atools::sql::SqlDatabase db;
    db.open("scenery.sqlite", options.isDebugAutocommit());

    db::Statement stmt = db.createStatement();
    stmt.executeScript(Glib::build_filename("sql", "create_nav_schema.sql"));
    stmt.executeScript(Glib::build_filename("sql", "create_ap_schema.sql"));
    stmt.executeScript(Glib::build_filename("sql", "create_meta_schema.sql"));
    stmt.executeScript(Glib::build_filename("sql", "create_views.sql"));
    db.commit();

    stmt.execute("PRAGMA foreign_keys = ON");
    db.commit();

    writer::DataWriter dataWriter(db, options);

    const AreaVectorType& areas = cfg.getAreas();
    for(AreaVectorIterType iter = areas.begin(); iter != areas.end(); ++iter)
    {
      const SceneryArea& area = *iter;
      if(area.isActive())
        dataWriter.writeSceneryArea(area);
    }
    db.commit();

    writer::RouteResolver resolver(db);
    resolver.run();
    db.commit();

    stmt.executeScript(Glib::build_filename("sql", "finish_schema.sql"));
    db.commit();

    dataWriter.logResults();
    db.createUtility().printTableStats();

    db.close();
  }
  catch(const db::DatabaseException& e)
  {
    log.errorStream() << "Caught db::DatabaseException " << e.what();
    retval = 1;
  }
  catch(const std::exception& e)
  {
    log.errorStream() << "Caught std::exception " << e.what();
    retval = 1;
  }
  catch(const Glib::Exception& e)
  {
    log.errorStream() << "Caught Glib::Exception: " << e.what();
    retval = 1;
  }
  catch(...)
  {
    log.errorStream() << "Caught other";
    retval = 1;
  }

  log.info("done.");
  log4::shutdownLogging();
#endif
  return retval;
}
