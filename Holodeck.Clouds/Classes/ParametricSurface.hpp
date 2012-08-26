#include "Interfaces.hpp"

struct ParametricInterval {
    ivec2 Divisions;
    vec2 UpperBound;
    vec2 TextureCount;
};

class ParametricSurface : public ISurface {
public:
    int GetVertexCount() const;
    int GetLineIndexCount() const;
    int GetTriangleIndexCount() const;
    void GenerateVertices(vector<float>& vertices, unsigned char flags) const;
    void GenerateLineIndices(vector<unsigned short>& indices) const;
    void GenerateTriangleIndices(vector<unsigned short>& indices) const;
protected:
    void SetInterval(const ParametricInterval& interval);
    virtual vec3 Evaluate(const vec2& domain) const = 0;
    virtual bool InvertNormal(const vec2& domain) const { return false; }
private:
    vec2 ComputeDomain(float i, float j) const;
    ivec2 m_slices;
    ivec2 m_divisions;
    vec2 m_upperBound;
    vec2 m_textureCount;
};

inline void ParametricSurface::SetInterval(const ParametricInterval& interval)
{
    m_divisions = interval.Divisions;
    m_upperBound = interval.UpperBound;
    m_textureCount = interval.TextureCount;
    m_slices = m_divisions - ivec2(1, 1);
}

inline int ParametricSurface::GetVertexCount() const
{
    return m_divisions.x * m_divisions.y;
}

inline int ParametricSurface::GetLineIndexCount() const
{
    return 4 * m_slices.x * m_slices.y;
}

inline int ParametricSurface::GetTriangleIndexCount() const
{
    return 6 * m_slices.x * m_slices.y;
}

inline vec2 ParametricSurface::ComputeDomain(float x, float y) const
{
    return vec2(x * m_upperBound.x / m_slices.x, y * m_upperBound.y / m_slices.y);
}

inline void
ParametricSurface::GenerateVertices(vector<float>& vertices,
                                    unsigned char flags) const
{
    int floatsPerVertex = 3;
    if (flags & VertexFlagsNormals)
        floatsPerVertex += 3;
    if (flags & VertexFlagsTexCoords)
        floatsPerVertex += 2;

    vertices.resize(GetVertexCount() * floatsPerVertex);
    float* attribute = &vertices[0];

    for (int j = 0; j < m_divisions.y; j++) {
        for (int i = 0; i < m_divisions.x; i++) {

            // Compute Position
            vec2 domain = ComputeDomain(i, j);
            vec3 range = Evaluate(domain);
            attribute = range.Write(attribute);

            // Compute Normal
            if (flags & VertexFlagsNormals) {
                float s = i, t = j;

                // Nudge the point if the normal is indeterminate.
                if (i == 0) s += 0.01f;
                if (i == m_divisions.x - 1) s -= 0.01f;
                if (j == 0) t += 0.01f;
                if (j == m_divisions.y - 1) t -= 0.01f;
                
                // Compute the tangents and their cross product.
                vec3 p = Evaluate(ComputeDomain(s, t));
                vec3 u = Evaluate(ComputeDomain(s + 0.01f, t)) - p;
                vec3 v = Evaluate(ComputeDomain(s, t + 0.01f)) - p;
                vec3 normal = u.Cross(v).Normalized();
                if (InvertNormal(domain))
                    normal = -normal;
                attribute = normal.Write(attribute);
            }
            
            // Compute Texture Coordinates
            if (flags & VertexFlagsTexCoords) {
                float s = m_textureCount.x * i / m_slices.x;
                float t = m_textureCount.y * j / m_slices.y;
                attribute = vec2(s, t).Write(attribute);
            }
        }
    }
}

inline void ParametricSurface::GenerateLineIndices(vector<unsigned short>& indices) const
{
    indices.resize(GetLineIndexCount());
    vector<unsigned short>::iterator index = indices.begin();
    for (int j = 0, vertex = 0; j < m_slices.y; j++) {
        for (int i = 0; i < m_slices.x; i++) {
            int next = (i + 1) % m_divisions.x;
            *index++ = vertex + i;
            *index++ = vertex + next;
            *index++ = vertex + i;
            *index++ = vertex + i + m_divisions.x;
        }
        vertex += m_divisions.x;
    }
}

inline void
ParametricSurface::GenerateTriangleIndices(vector<unsigned short>& indices) const
{
    indices.resize(GetTriangleIndexCount());
    vector<unsigned short>::iterator index = indices.begin();
    for (int j = 0, vertex = 0; j < m_slices.y; j++) {
        for (int i = 0; i < m_slices.x; i++) {
            int next = (i + 1) % m_divisions.x;
            *index++ = vertex + i;
            *index++ = vertex + next;
            *index++ = vertex + i + m_divisions.x;
            *index++ = vertex + next;
            *index++ = vertex + next + m_divisions.x;
            *index++ = vertex + i + m_divisions.x;
        }
        vertex += m_divisions.x;
    }
}

class Quad : public ParametricSurface {
public:
    Quad(float size, float reps = 1) : m_size(size, size)
    {
        ParametricInterval interval = { ivec2(2, 2), vec2(1, 1), vec2(reps, reps) };
        SetInterval(interval);
    }
    vec3 Evaluate(const vec2& domain) const
    {
        float x = (domain.x - 0.5) * m_size.x;
        float y = (domain.y - 0.5) * m_size.y;
        return vec3(x, -y, 0);
    }
private:
    vec2 m_size;
};

class Sphere : public ParametricSurface {
public:
    Sphere(float radius, int tess = 40, vec2 textureCount = vec2(1, 1)) : m_radius(radius)
    {
        ParametricInterval interval = { ivec2(tess, tess), vec2(Pi, TwoPi), textureCount };
        SetInterval(interval);
    }
    vec3 Evaluate(const vec2& domain) const
    {
        float u = domain.x, v = domain.y;
        float x = m_radius * sin(u) * cos(v);
        float y = m_radius * cos(u);
        float z = m_radius * -sin(u) * sin(v);
        return vec3(x, y, z);
    }
private:
    float m_radius;
};
