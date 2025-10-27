#pragma once
#include <string>
#include <vector>
#include <functional>

struct DbSceneObject
{
    std::string type;
    float sx, sy, sz;
    float rx, ry, rz;
    float px, py, pz;
    float cr, cg, cb, ca;
    std::string materialTag;
    std::string textureTag;
    float uScale, vScale;
};

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    bool open(const std::string& path);
    void close();

    bool initSchema();

    bool clearScene();
    bool insertObject(const DbSceneObject& obj);
    bool loadScene(std::vector<DbSceneObject>& out);
    bool updateFirstObjectPosition(float x, float y, float z); // small example of update
    bool deleteLastObject();                                   // small example of delete

private:
    struct sqlite3* m_db;
    bool exec(const std::string& sql);
};
