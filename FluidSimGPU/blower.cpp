#include "blower.h"

Blower::Blower(glm::vec2 _sourcePosition, float _sourceWidth, float _length, float _endWidth,
	float _strength, float _angle, Shader* _predictShader) {

	//set fields
	sourcePosition = _sourcePosition;
	sourceWidth = _sourceWidth;
	length = _length;
	endWidth = _endWidth;
	strength = _strength;
	angle = _angle;

	//setup buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), &vertexIndices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 4)); //vertex color
	glEnableVertexAttribArray(1);
	glBindVertexArray(0); //done!

	setupBlower(_predictShader);
}

void Blower::setSource(Shader* predictShader, glm::vec2 source) {
	sourcePosition = source;
	setupBlower(predictShader);
}

void Blower::setEnd(Shader* predictShader, glm::vec2 end) {
	glm::vec2 lengthVec = end - sourcePosition;
	length = std::max(glm::length(lengthVec), 50.0f);
	angle = -atan2(lengthVec.x, lengthVec.y); //normalise length vec?
	setupBlower(predictShader);
}

//From Gareth Rees' answer:
//https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
void Blower::findCentre(glm::vec2 start1, glm::vec2 end1, glm::vec2 start2, glm::vec2 end2) {
	glm::vec2 p = start1;
	glm::vec2 r = end1 - start1;
	glm::vec2 q = start2;
	glm::vec2 s = end2 - start2;

	float a = r.x * s.y - r.y * s.x;
	float b = (q - p).x * r.y - (q - p).y * r.x;

	//if (a == 0 && b != 0) { //parallel
	//	parallel = true;
	//	behindEnd = false;
	//	return;
	//}
	
	//intersection!
	float u = b / a;
	centre = q + u * s;
	parallel = false;
	behindEnd = u > 0;
}

void Blower::deleteBuffers() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void Blower::completeSetup(Shader* predictShader) {
	//setups up buffers again, used for when we load it from a file
	//setup buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), &vertexIndices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 4)); //vertex color
	glEnableVertexAttribArray(1);
	glBindVertexArray(0); //done!

	setupBlower(predictShader);
}

void Blower::setupBlower(Shader* predictShader) {
	//generate vertices
	glm::vec2 vAngle = glm::vec2(cos(angle), sin(angle)); //assumes angle is in radians
	//garuanteed to be a unit vector because sin^2 + cos^2 = 1
	glm::vec2 v1 = sourcePosition + (float)(0.5 * sourceWidth) * vAngle;
	glm::vec2 v2 = sourcePosition - (float)(0.5 * sourceWidth) * vAngle;

	glm::vec2 blowerDir = glm::vec2(-vAngle.y, vAngle.x); //right hand perpendicular
	glm::vec2 endCentre = sourcePosition + length * blowerDir; //right hand perpendicular
	glm::vec2 v3 = endCentre + (float)(0.5 * endWidth) * vAngle;
	glm::vec2 v4 = endCentre - (float)(0.5 * endWidth) * vAngle;

	//determine circle centre (for getting direction of force)
	//check if parallel first
	if (sourceWidth == endWidth) {
		parallel = true;
		behindEnd = false;
	}
	else findCentre(v1, v3, v2, v4);

	std::cout << centre.x << " " << centre.y << std::endl;
	std::cout << "parallel" << parallel << " behindEnd" << behindEnd << std::endl;

	//determine AABB
	float maxX = std::max(std::max(v1.x, v2.x), std::max(v3.x, v4.x));
	float maxY = std::max(std::max(v1.y, v2.y), std::max(v3.y, v4.y));
	float minX = std::min(std::min(v1.x, v2.x), std::min(v3.x, v4.x));
	float minY = std::min(std::min(v1.y, v2.y), std::min(v3.y, v4.y));
	glm::vec2 AABBmax = glm::vec2(maxX, maxY);
	glm::vec2 AABBmin = glm::vec2(minX, minY);

	//generate vertex data
	vertexData[0] = glm::vec4(v1, 0, 1); vertexData[1] = glm::vec4(0, 0, 0, 0.8);
	vertexData[2] = glm::vec4(v2, 0, 1); vertexData[3] = glm::vec4(0, 0, 0, 0.8);
	vertexData[4] = glm::vec4(v3, 0, 1); vertexData[5] = glm::vec4(0, 0, 0, 0);
	vertexData[6] = glm::vec4(v4, 0, 1); vertexData[7] = glm::vec4(0, 0, 0, 0);

	//update opengl stuff
	updateGPU(predictShader, blowerDir, AABBmin, AABBmax);
}

void Blower::updateGPU(Shader* predictShader, glm::vec2 blowerDir, glm::vec2 AABBmin, glm::vec2 AABBmax) {
	//update vertex data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData[0], GL_STATIC_DRAW);

	//update predict shader parameters
	predictShader->use();
	predictShader->setVec2("blowerDir", blowerDir);
	predictShader->setVec2("blowerCentre", centre);
	predictShader->setFloat("blowerStrength", strength);
	predictShader->setVec2("sourcePos", sourcePosition);
	predictShader->setVec2("blowerLengthVec", blowerDir * length);
	predictShader->setFloat("blowerLength", length);
	predictShader->setVec2("bV1", glm::vec2(vertexData[0].x, vertexData[0].y));
	predictShader->setVec2("bV2", glm::vec2(vertexData[2].x, vertexData[2].y));
	predictShader->setVec2("bV3", glm::vec2(vertexData[4].x, vertexData[4].y));
	predictShader->setVec2("bV4", glm::vec2(vertexData[6].x, vertexData[6].y));
	predictShader->setVec2("AABBmin", AABBmin);
	predictShader->setVec2("AABBmax", AABBmax);
	predictShader->setBool("blowerOn", on);
	predictShader->setBool("parallel", parallel);
	predictShader->setBool("behindEnd", behindEnd);
}

void Blower::render(Shader* pointShader) {
	pointShader->use();
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

//template<class Archive> void Blower::serialize(Archive& archive) {
//	archive(sourcePosition, centre, length, angle, parallel, behindEnd, vertexData,
//		vertexIndices, VAO, EBO, VBO, sourceWidth, endWidth, strength, on, visible);
//}