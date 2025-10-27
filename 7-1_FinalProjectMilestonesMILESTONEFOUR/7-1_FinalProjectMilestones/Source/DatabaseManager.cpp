#include "DatabaseManager.h"
#include "sqlite3.h"
#include <iostream>

DatabaseManager::DatabaseManager() : m_db(nullptr) {}
DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::open(const std::string& path)
{
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK)
    {
        std::cerr << "SQLite open failed: " << sqlite3_errmsg(m_db) << std::endl;
        m_db = nullptr;
        return false;
    }
    return true;
}

void DatabaseManager::close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool DatabaseManager::exec(const std::string& sql)
{
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQLite exec error: " << (err ? err : "unknown") << std::endl;
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool DatabaseManager::initSchema()
{
    const char* ddl =
        "CREATE TABLE IF NOT EXISTS scene_object ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " type TEXT NOT NULL,"
        " sx REAL, sy REAL, sz REAL,"
        " rx REAL, ry REAL, rz REAL,"
        " px REAL, py REAL, pz REAL,"
        " color_r REAL, color_g REAL, color_b REAL, color_a REAL,"
        " material_tag TEXT,"
        " texture_tag TEXT,"
        " u_scale REAL,"
        " v_scale REAL"
        ");";
    return exec(ddl);
}

bool DatabaseManager::clearScene()
{
    return exec("DELETE FROM scene_object;");
}

bool DatabaseManager::insertObject(const DbSceneObject& o)
{
    // using prepared statements for safety and performance
    const char* sql =
        "INSERT INTO scene_object(type,sx,sy,sz,rx,ry,rz,px,py,pz,color_r,color_g,color_b,color_a,material_tag,texture_tag,u_scale,v_scale)"
        " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    int idx = 1;
    sqlite3_bind_text(stmt, idx++, o.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, idx++, o.sx); sqlite3_bind_double(stmt, idx++, o.sy); sqlite3_bind_double(stmt, idx++, o.sz);
    sqlite3_bind_double(stmt, idx++, o.rx); sqlite3_bind_double(stmt, idx++, o.ry); sqlite3_bind_double(stmt, idx++, o.rz);
    sqlite3_bind_double(stmt, idx++, o.px); sqlite3_bind_double(stmt, idx++, o.py); sqlite3_bind_double(stmt, idx++, o.pz);
    sqlite3_bind_double(stmt, idx++, o.cr); sqlite3_bind_double(stmt, idx++, o.cg); sqlite3_bind_double(stmt, idx++, o.cb); sqlite3_bind_double(stmt, idx++, o.ca);
    sqlite3_bind_text(stmt, idx++, o.materialTag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, o.textureTag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, idx++, o.uScale); sqlite3_bind_double(stmt, idx++, o.vScale);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseManager::loadScene(std::vector<DbSceneObject>& out)
{
    out.clear();
    const char* sql = "SELECT type,sx,sy,sz,rx,ry,rz,px,py,pz,color_r,color_g,color_b,color_a,material_tag,texture_tag,u_scale,v_scale FROM scene_object;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        DbSceneObject o;
        int c = 0;
        o.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c++));
        o.sx = (float)sqlite3_column_double(stmt, c++); o.sy = (float)sqlite3_column_double(stmt, c++); o.sz = (float)sqlite3_column_double(stmt, c++);
        o.rx = (float)sqlite3_column_double(stmt, c++); o.ry = (float)sqlite3_column_double(stmt, c++); o.rz = (float)sqlite3_column_double(stmt, c++);
        o.px = (float)sqlite3_column_double(stmt, c++); o.py = (float)sqlite3_column_double(stmt, c++); o.pz = (float)sqlite3_column_double(stmt, c++);
        o.cr = (float)sqlite3_column_double(stmt, c++); o.cg = (float)sqlite3_column_double(stmt, c++); o.cb = (float)sqlite3_column_double(stmt, c++); o.ca = (float)sqlite3_column_double(stmt, c++);
        o.materialTag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c++));
        o.textureTag  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c++));
        o.uScale = (float)sqlite3_column_double(stmt, c++); o.vScale = (float)sqlite3_column_double(stmt, c++);
        out.push_back(o);
    }
    sqlite3_finalize(stmt);
    return true;
}

bool DatabaseManager::updateFirstObjectPosition(float x, float y, float z)
{
    const char* sql = "UPDATE scene_object SET px=?, py=?, pz=? WHERE id=(SELECT MIN(id) FROM scene_object);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_double(stmt, 1, x);
    sqlite3_bind_double(stmt, 2, y);
    sqlite3_bind_double(stmt, 3, z);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseManager::deleteLastObject()
{
    return exec("DELETE FROM scene_object WHERE id=(SELECT MAX(id) FROM scene_object);");
}
