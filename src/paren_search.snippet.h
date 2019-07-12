static
bool FUNCTION_NAME(struct parser* p, enum paren_types type) {
	switch(type) {
	case C_COMMENT:
		if(!DOSTRING(p, LITSTR("*/"))) return false;
		break;
	default: {
		unsigned char thechar = 0;
		switch(type) {
		case C_LINE_COMMENT:
			thechar = '\n';
			break;				
		case PAREN:
			thechar = ')';
			break;
		case SQUARE_BRACKET:
			thechar = ']';
			break;
		case CURLY_BRACKET:
			thechar = '}';
			break;
		case ANGLE_BRACKET:
			thechar = '>';
			break;
		case DOUBLEQUOTE:
			thechar = '"';
			break;
		default:
			ERROR("what? %d", type);
		};
		if(!DOCHAR(p, thechar)) return false;
	}
	};
}

#undef FUNCTION_NAME
#undef DOSTRING
#undef DOCHAR
