#include "line.h"

Line::Line(glm::vec2 v1, glm::vec2 v2) {
	a = v1;
	b = v2;
	id = 0;
	ab = b - a;
	length = glm::distance(a, b);
}

//template<class Archive> void Line::serialize(Archive& archive) {
//	archive(a, b, ab, length, id);
//}