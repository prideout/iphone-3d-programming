#include <list>

class SpringNode;

typedef std::list<SpringNode*> NeighborList;

class SpringNode {
public:
    SpringNode()
    {
        m_position = vec2(0, 0);
        m_velocity = vec2(0, 0);
        m_mass = 1;
        m_pinned = false;
    }
    void Pin()
    {
        m_pinned = true;
    }
    void SetPosition(const vec2& p)
    {
        m_position = p;
    }
    vec2 GetPosition() const
    {
        return m_position;
    }
    void AddNeighbor(SpringNode* node)
    {
        m_neighbors.push_back(node);
    }
    void ResetForce(vec2 force)
    {
        m_force = force;
    }
    void ComputeForce()
    {
        const float StiffnessConstant = 3.0f;
        const float RestLength = 0.075f;
        const float DampingConstant = 2.0f;
        
        NeighborList::const_iterator n = m_neighbors.begin();
        for (; n != m_neighbors.end(); ++n) {

            // Compute the spring force:
            vec2 v = (*n)->m_position - m_position;
            float length = v.Length();
            vec2 direction = v.Normalized();
            vec2 restoringForce = direction * StiffnessConstant * (length - RestLength);

            // Compute the damping force:
            vec2 relativeVelocity = (*n)->m_velocity - m_velocity;
            vec2 dampingForce = relativeVelocity * DampingConstant;
            
            // Add the two forces to this node and subtract them from the neighbor:
            vec2 totalForce = restoringForce + dampingForce;
            m_force += totalForce;
            (*n)->m_force -= totalForce;
        }
    }
    void Update(float dt)
    {
        if (m_pinned)
            return;
        
        vec2 acceleration = m_force / m_mass;
        m_velocity += acceleration * dt;
        m_position += m_velocity * dt;
    }
private:
    vec2 m_force;
    vec2 m_position;
    vec2 m_velocity;
    float m_mass;
    bool m_pinned;
    NeighborList m_neighbors;
};
