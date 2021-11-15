#include "line.h"

Line::Line(glm::vec2 v1, glm::vec2 v2) {
	a = v1;
	b = v2;
	id = 0;
	ab = b - a;
	length = glm::distance(a, b);
}

glm::vec2 Line::particleCollision(Particle* p, float r, float damping) {
	glm::vec2 c = p->position;
	glm::vec2 ac = c - a; //c is the centre of the circle

	//project ac onto ab to get ad
	//glm::vec2 ad = ((ac.dot(ab)) / (ab.dot(ab))) * ab;
	glm::vec2 ad = (glm::dot(ac, ab) / glm::dot(ab, ab)) * ab;

	//check that d lies between a and b
	//without this check the collision would be between the particle and this line but infinite
	glm::vec2 d = a + ad;
	float dotProduct = glm::dot(ab, ad); //ab.dot(ad);
	if (dotProduct < 0 || dotProduct > length * length) return glm::vec2(0, 0);

	//check for collision
	glm::vec2 dc = c - d;
	float dcLength2 = glm::dot(dc, dc); //dc.lengthSquared();
	if (dcLength2 < r * r) { //collision
		//resolve collision
		glm::vec2 normal = glm::normalize(dc); //dc.normalised();
		float dcLength = sqrt(dcLength2);
		float amountToAdd = r - dcLength;
		p->position += normal * amountToAdd;

		//reflect velocity across normal
		//glm::vec2 v = (p->velocity - 2 * (p->velocity.dot(normal)) * normal); //* abs(damping);
		glm::vec2 v = (p->velocity - 2 * (glm::dot(p->velocity, normal)) * normal);

		//split into components along normal and ab
		float normalComponent = glm::dot(v, normal); //v.dot(normal); //dont need to divide by length as normal is normalised
		float lineComponent = glm::dot(v, ab) / length; //v.dot(ab) / length;

		//dampen normal component
		normalComponent *= abs(damping);

		//could optionally apply friction to the lineComponent

		//reconstruct dampened velocity and assign
		v = (normalComponent * normal) + (lineComponent * ab / length);
		p->velocity = v;
	}

	return ad;
}