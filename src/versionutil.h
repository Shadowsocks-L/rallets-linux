#ifndef VERSIONUTIL_H
#define VERSIONUTIL_H

// https://sourcey.com/comparing-version-strings-in-cpp/

struct Version
{
    int major = 0, minor = 0, revision = 0, build = 0;

    Version(const std::string& version)
    {
        std::sscanf(version.c_str(), "%d.%d.%d.%d", &major, &minor, &revision, &build);
        if (major < 0) major = 0;
        if (minor < 0) minor = 0;
        if (revision < 0) revision = 0;
        if (build < 0) build = 0;
    }

    bool operator < (const Version& other)
    {
        if (major < other.major)
            return true;
        if (minor < other.minor)
            return true;
        if (revision < other.revision)
            return true;
        if (build < other.build)
            return true;
        return false;
    }

    bool operator == (const Version& other)
    {
        return major == other.major
            && minor == other.minor
            && revision == other.revision
            && build == other.build;
    }

    friend std::ostream& operator << (std::ostream& stream, const Version& ver)
    {
        stream << ver.major;
        stream << '.';
        stream << ver.minor;
        stream << '.';
        stream << ver.revision;
        stream << '.';
        stream << ver.build;
        return stream;
    }
};

#endif // VERSIONUTIL_H

