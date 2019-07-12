static
bool FUNCTION_NAME(struct parser* p, unsigned char i) {
	switch(p->parens[i]) {
	case C_COMMENT:
		if(!DOSTRING(p, "*/")) return false;
		break;
	default: {
		unsigned char thechar = 0;
		switch(p->parens[i]) {
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
			ERROR("what?");
		};
		if(!DOCHAR(p, thechar)) return false;
	}
	};
}
