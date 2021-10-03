#include "line.h"

Line::Line(glm::vec2 v1, glm::vec2 v2, Shader* _shader) {
	if (v1.x < v2.x || v1.y < v2.y) { //order vertices correctly, may be unncessary
		a = v1;
		b = v2;
	}
	else {
		a = v2;
		b = v1;
	}
	if (abs(a.x - b.x) == abs(a.y - b.y)) angleOf45 = true;
	else angleOf45 = false;
	ab = b - a;
	length = glm::length(ab); //(ab).length();

	//generate vertices to create thick line
	glm::vec2 perp(ab.y, -ab.x);
	perp = glm::normalize(perp);
	float widthScale = constants::LINE_WIDTH / 2.0f;
	perp *= widthScale;
	glm::vec2 vert1 = v1 + perp;
	glm::vec2 vert2 = v1 - perp;
	glm::vec2 vert3 = v2 + perp;
	glm::vec2 vert4 = v2 - perp;

	float vertices[] = {
		vert1.x, vert1.y,
		vert2.x, vert2.y,
		vert3.x, vert3.y,
		vert4.x, vert4.y
	};

	shader = _shader;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//unbind?
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

void Line::render() {
	shader->use();
	glm::vec3 color(0.0f, 0.0f, 0.0f); //black
	glUniform3fv(glGetUniformLocation(shader->id, "color"), 1, glm::value_ptr(color));
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Line::close() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}