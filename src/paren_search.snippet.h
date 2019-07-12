#define ONE(type,char) type: return DOCHAR(p, char)

static
bool FUNCTION_NAME(struct parser* p, enum paren_types type, enum paren_direction direction) {
	if(direction == OPEN_PAREN) {
		switch(type) {
		case C_LINE_COMMENT:
			return DOSTRING(p, LITSTR("//"));
		case C_COMMENT:
			return DOSTRING(p, LITSTR("/*"));
		case ONE(PAREN, '(');
		case ONE(SQUARE_BRACKET, '[');
		case ONE(CURLY_BRACKET, '{');
		case ONE(ANGLE_BRACKET, '<');
		case ONE(DOUBLEQUOTE, '"');
		default:
			ERROR("huh? %d", type);
		};
	} else {
		switch(type) {
		case C_COMMENT:
			return DOSTRING(p, LITSTR("*/"));
		case ONE(C_LINE_COMMENT, '\n');
		case ONE(PAREN, ')');
		case ONE(SQUARE_BRACKET, ']');
		case ONE(CURLY_BRACKET, '}');
		case ONE(ANGLE_BRACKET, '>');
		case ONE(DOUBLEQUOTE, '"');
		default:
			ERROR("huh?? %d", type);
		};			
	}
	ERROR("Should never get here");
}

#undef FUNCTION_NAME
#undef DOSTRING
#undef DOCHAR
#undef ONE
