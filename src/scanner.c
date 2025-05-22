#include "scanner.h"
#include <ctype.h>
#include <stdint.h>
#include <string.h>

static inline bool _is_at_end(struct scanner scanner) {
  return *scanner.current == 0;
}

static struct token _make_token(struct scanner scanner, enum token_kind kind) {
  struct token token;
  token.kind = kind;
  token.start = scanner.start;
  token.length = (uint32_t)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static struct token _error_token(struct scanner scanner, const char *message) {
  struct token token;
  token.kind = TOKEN_KIND_SP_ERROR;
  token.start = message;
  token.length = (int32_t)strlen(message);
  token.line = scanner.line;
  return token;
}

static char _advance(struct scanner *scanner) {
  scanner->current++;
  return scanner->current[-1];
}

static bool _match(struct scanner *scanner, char expected) {
  if (*scanner->current != expected) {
    return false;
  }
  scanner->current++;
  return true;
}

static inline char _peek(struct scanner scanner) { return *scanner.current; }

static inline char _peek_next(struct scanner scanner) {
  if (_is_at_end(scanner)) {
    return 0;
  }
  return scanner.current[1];
}

static void _skip_whitespace(struct scanner *scanner) {
  for (;;) {
    char c = _peek(*scanner);
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      _advance(scanner);
      break;
    case '/':
      if (_peek_next(*scanner) == '/') {
        while (_peek(*scanner) != '\n' && !_is_at_end(*scanner)) {
          _advance(scanner);
        }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static struct token _make_string(struct scanner *scanner) {
  while (_peek(*scanner) != '"' && !_is_at_end(*scanner)) {
    if (_peek(*scanner) == '\n')
      ++scanner->line;
    _advance(scanner);
  }

  if (_is_at_end(*scanner)) {
    return _error_token(*scanner, "Unterminated string.");
  }

  _advance(scanner);
  return _make_token(*scanner, TOKEN_KIND_LT_STRING);
}

static struct token make_char(struct scanner *scanner) {
  while (_peek(*scanner) != '\'' && !_is_at_end(*scanner)) {
    if (_peek(*scanner) == '\n')
      ++scanner->line;
    _advance(scanner);
  }

  if (_is_at_end(*scanner)) {
    return _error_token(*scanner, "Unterminated character literal.");
  }

  _advance(scanner);

  if (scanner->current - scanner->start == 2) {
    return _error_token(*scanner, "Empty character literal.");
  }
  if (scanner->current - scanner->start > 4) {
    return _error_token(*scanner, "Multi-character character literal.");
  }

  return _make_token(*scanner, TOKEN_KIND_LT_CHAR);
}

static struct token _make_number(struct scanner *scanner) {
  while (isdigit(_peek(*scanner))) {
    _advance(scanner);
  }

  bool is_real = false;
  if (_peek(*scanner) == '.' && isdigit(_peek_next(*scanner))) {
    is_real = true;
    _advance(scanner);
    while (isdigit(_peek(*scanner))) {
      _advance(scanner);
    }
  }

  return is_real ? _make_token(*scanner, TOKEN_KIND_LT_REAL)
                 : _make_token(*scanner, TOKEN_KIND_LT_INTEGER);
}

static enum token_kind _check_keyword(struct scanner scanner, uint32_t start,
                                      uint32_t length, const char *rest,
                                      enum token_kind kind) {
  return scanner.current - scanner.start == start + length &&
                 !memcmp(scanner.start + start, rest, length)
             ? kind
             : TOKEN_KIND_SP_IDENT;
}

static enum token_kind _make_identifier_kind(struct scanner scanner) {
  const uint16_t length = scanner.current - scanner.start;

  switch (scanner.start[0]) {
  case 'A':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'N':
        return _check_keyword(scanner, 2, 1, "D", TOKEN_KIND_KW_AND);
      case 'P':
        return _check_keyword(scanner, 2, 4, "PEND", TOKEN_KIND_KW_APPEND);
      case 'R':
        return _check_keyword(scanner, 2, 3, "RAY", TOKEN_KIND_KW_ARRAY);
      }
    }
    break;
  case 'B':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'O':
        return _check_keyword(scanner, 2, 5, "OLEAN", TOKEN_KIND_KW_BOOLEAN);
      case 'Y':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'V':
            return _check_keyword(scanner, 3, 2, "AL", TOKEN_KIND_KW_BYVAL);
          case 'R':
            return _check_keyword(scanner, 3, 2, "EF", TOKEN_KIND_KW_BYREF);
          }
        }
      }
    }
    break;
  case 'C':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'A':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'L':
            return _check_keyword(scanner, 3, 1, "L", TOKEN_KIND_KW_CALL);
          case 'S':
            return _check_keyword(scanner, 3, 1, "E", TOKEN_KIND_KW_CASE);
          }
        }
        break;
      case 'H':
        return _check_keyword(scanner, 2, 2, "AR", TOKEN_KIND_KW_CHAR);
      case 'L':
        if (length > 1) {
          switch (scanner.start[2]) {
          case 'A':
            return _check_keyword(scanner, 3, 2, "SS", TOKEN_KIND_KW_CLASS);
          case 'O':
            return _check_keyword(scanner, 3, 6, "SEFILE",
                                  TOKEN_KIND_KW_CLOSEFILE);
          }
        }
        break;
      case 'O':
        return _check_keyword(scanner, 2, 6, "NSTANT", TOKEN_KIND_KW_CONSTANT);
      }
    }
    break;
  case 'D':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'A':
        return _check_keyword(scanner, 2, 2, "TE", TOKEN_KIND_KW_DATE);
      case 'E':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'C':
            return _check_keyword(scanner, 3, 4, "LARE", TOKEN_KIND_KW_DECLARE);
          case 'F':
            return _check_keyword(scanner, 3, 3, "INE", TOKEN_KIND_KW_DEFINE);
          }
        }
        break;
      case 'I':
        return _check_keyword(scanner, 2, 1, "V", TOKEN_KIND_KW_DIV);
      }
    }
    break;
  case 'E':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'L':
        return _check_keyword(scanner, 2, 2, "SE", TOKEN_KIND_KW_ELSE);
      case 'N':
        if (length > 3 && scanner.start[2] == 'D') {
          switch (scanner.start[3]) {
          case 'C':
            if (length > 4) {
              switch (scanner.start[4]) {
              case 'A':
                return _check_keyword(scanner, 5, 2, "SE",
                                      TOKEN_KIND_KW_ENDCASE);
              case 'L':
                return _check_keyword(scanner, 5, 3, "ASS",
                                      TOKEN_KIND_KW_ENDCLASS);
              }
            }
            break;
          case 'F':
            return _check_keyword(scanner, 4, 7, "UNCTION",
                                  TOKEN_KIND_KW_ENDFUNCTION);
          case 'I':
            return _check_keyword(scanner, 4, 1, "F", TOKEN_KIND_KW_ENDIF);
          case 'P':
            return _check_keyword(scanner, 4, 8, "ROCEDURE",
                                  TOKEN_KIND_KW_ENDPROCEDURE);
          case 'T':
            return _check_keyword(scanner, 4, 3, "YPE", TOKEN_KIND_KW_ENDTYPE);
          case 'W':
            return _check_keyword(scanner, 4, 4, "HILE",
                                  TOKEN_KIND_KW_ENDWHILE);
          }
        }
      }
    }
    break;
  case 'F':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'A':
        return _check_keyword(scanner, 2, 3, "LSE", TOKEN_KIND_LT_FALSE);
      case 'O':
        return _check_keyword(scanner, 2, 1, "R", TOKEN_KIND_KW_FOR);
      case 'U':
        return _check_keyword(scanner, 2, 6, "NCTION", TOKEN_KIND_KW_FUNCTION);
      }
    }
    break;
  case 'G':
    return _check_keyword(scanner, 1, 8, "ETRECORD", TOKEN_KIND_KW_GETRECORD);
  case 'I':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'F':
        return TOKEN_KIND_KW_IF;
      case 'N':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'H':
            return _check_keyword(scanner, 3, 5, "ERITS",
                                  TOKEN_KIND_KW_INHERITS);
          case 'P':
            return _check_keyword(scanner, 3, 2, "UT", TOKEN_KIND_KW_INPUT);
          case 'T':
            return _check_keyword(scanner, 3, 4, "EGER", TOKEN_KIND_KW_INTEGER);
          }
        }
      }
    }
    break;
  case 'M':
    return _check_keyword(scanner, 1, 2, "OD", TOKEN_KIND_KW_MOD);
  case 'N':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'E':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'X':
            return _check_keyword(scanner, 3, 1, "T", TOKEN_KIND_KW_NEXT);
          case 'W':
            return TOKEN_KIND_KW_NEW;
          }
        }
      case 'O':
        return _check_keyword(scanner, 2, 1, "T", TOKEN_KIND_KW_NOT);
      }
    }
    break;
  case 'O':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'P':
        return _check_keyword(scanner, 2, 6, "ENFILE", TOKEN_KIND_KW_OPENFILE);
      case 'R':
        return TOKEN_KIND_KW_OR;
      case 'T':
        return _check_keyword(scanner, 2, 7, "HERWISE",
                              TOKEN_KIND_KW_OTHERWISE);
      case 'U':
        return _check_keyword(scanner, 2, 4, "TPUT", TOKEN_KIND_KW_OUTPUT);
      }
    }
    break;
  case 'P':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'R':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'O':
            return _check_keyword(scanner, 3, 6, "CEDURE",
                                  TOKEN_KIND_KW_PROCEDURE);
          case 'I':
            return _check_keyword(scanner, 3, 4, "VATE", TOKEN_KIND_KW_PRIVATE);
          }
          break;
        }
      case 'U':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'B':
            return _check_keyword(scanner, 3, 3, "LIC", TOKEN_KIND_KW_PUBLIC);
          case 'T':
            return _check_keyword(scanner, 3, 6, "RECORD",
                                  TOKEN_KIND_KW_PUTRECORD);
          }
          break;
        }
      }
    }
    break;
  case 'R':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'A':
        return _check_keyword(scanner, 2, 4, "NDOM", TOKEN_KIND_KW_RANDOM);
      case 'E':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'A':
            if (length > 3) {
              switch (scanner.start[3]) {
              case 'D':
                if (length > 4) {
                  return _check_keyword(scanner, 4, 4, "FILE",
                                        TOKEN_KIND_KW_READFILE);
                }
                return _check_keyword(scanner, 4, 0, "", TOKEN_KIND_KW_READ);
              case 'L':
                return _check_keyword(scanner, 4, 0, "", TOKEN_KIND_KW_REAL);
                break;
              }
            }
            break;
          case 'P':
            return _check_keyword(scanner, 3, 3, "EAT", TOKEN_KIND_KW_REPEAT);
          case 'T':
            if (length == 6) {
              return _check_keyword(scanner, 3, 3, "URN", TOKEN_KIND_KW_RETURN);
            }
            if (length > 6) {
              return _check_keyword(scanner, 3, 4, "URNS",
                                    TOKEN_KIND_KW_RETURNS);
            }
          }
        }
        break;
      }
    }
    break;
  case 'S':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'E':
        return _check_keyword(scanner, 2, 2, "EK", TOKEN_KIND_KW_SEEK);
      case 'T':
        if (length > 2) {
          switch (scanner.start[2]) {
          case 'E':
            return _check_keyword(scanner, 3, 1, "P", TOKEN_KIND_KW_STEP);
          case 'R':
            return _check_keyword(scanner, 3, 3, "ING", TOKEN_KIND_KW_STRING);
          }
        }
        break;
      case 'U':
        return _check_keyword(scanner, 2, 3, "PER", TOKEN_KIND_KW_SUPER);
      }
    }
    break;
  case 'T':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'H':
        return _check_keyword(scanner, 2, 2, "EN", TOKEN_KIND_KW_THEN);
      case 'R':
        return _check_keyword(scanner, 2, 2, "UE", TOKEN_KIND_LT_TRUE);
      case 'Y':
        return _check_keyword(scanner, 2, 2, "PE", TOKEN_KIND_KW_TYPE);
      }
    }
    break;
  case 'U':
    return _check_keyword(scanner, 1, 4, "NTIL", TOKEN_KIND_KW_UNTIL);
  case 'W':
    if (length > 1) {
      switch (scanner.start[1]) {
      case 'H':
        return _check_keyword(scanner, 2, 3, "ILE", TOKEN_KIND_KW_WHILE);
      case 'R':
        if (length == 5) {
          return _check_keyword(scanner, 2, 3, "ITE", TOKEN_KIND_KW_WRITE);
        }
        if (length > 5) {
          return _check_keyword(scanner, 2, 7, "ITEFILE",
                                TOKEN_KIND_KW_WRITEFILE);
        }
      }
    }
    break;
  }
  return TOKEN_KIND_SP_IDENT;
}

static struct token _make_identifier(struct scanner *scanner) {
  while (isalnum(_peek(*scanner)) || _peek(*scanner) == '_') {
    _advance(scanner);
  }
  return _make_token(*scanner, _make_identifier_kind(*scanner));
}

struct token scanner_scan_token(struct scanner *scanner) {
  _skip_whitespace(scanner);

  scanner->start = scanner->current;

  if (_is_at_end(*scanner)) {
    return _make_token(*scanner, TOKEN_KIND_SP_EOF);
  }

  char c = _advance(scanner);

  if (isalpha(c) || c == '_') {
    return _make_identifier(scanner);
  }

  if (isdigit(c)) {
    return _make_number(scanner);
  }

  switch (c) {
  case '\n':
    ++scanner->line;
    return _make_token(*scanner, TOKEN_KIND_SP_EOL);
  case '-':
    return _make_token(*scanner, TOKEN_KIND_OP_SUBTRACTION);
  case '*':
    return _make_token(*scanner, TOKEN_KIND_OP_MULTIPLICATION);
  case '/':
    return _make_token(*scanner, TOKEN_KIND_OP_DIVISION);
  case '+':
    return _make_token(*scanner, TOKEN_KIND_OP_ADDITION);
  case '<':
    if (_match(scanner, '-')) {
      return _make_token(*scanner, TOKEN_KIND_OP_ASSIGN);
    }
    if (_match(scanner, '=')) {
      return _make_token(*scanner, TOKEN_KIND_OP_LESS_OR_EQUAL_TO);
    }
    if (_match(scanner, '>')) {
      return _make_token(*scanner, TOKEN_KIND_OP_NOT_EQUAL_TO);
    }
    return _make_token(*scanner, TOKEN_KIND_OP_LESS_THAN);
  case '=':
    return _make_token(*scanner, TOKEN_KIND_OP_EQUAL_TO);
  case '>':
    if (_match(scanner, '=')) {
      return _make_token(*scanner, TOKEN_KIND_OP_GREATER_OR_EQUAL_TO);
    }
    return _make_token(*scanner, TOKEN_KIND_OP_GREATER_THAN);
  case '^':
    return _make_token(*scanner, TOKEN_KIND_OP_CARET);
  case '&':
    return _make_token(*scanner, TOKEN_KIND_OP_CONCAT);
  case ':':
    return _make_token(*scanner, TOKEN_KIND_OP_COLON);
  case ',':
    return _make_token(*scanner, TOKEN_KIND_OP_COMMA);
  case '(':
    return _make_token(*scanner, TOKEN_KIND_OP_PAREN_OPEN);
  case ')':
    return _make_token(*scanner, TOKEN_KIND_OP_PAREN_CLOSE);
  case '[':
    return _make_token(*scanner, TOKEN_KIND_OP_BRACKET_OPEN);
  case ']':
    return _make_token(*scanner, TOKEN_KIND_OP_BRACKET_CLOSE);
  case '.':
    return _make_token(*scanner, TOKEN_KIND_OP_DOT);
  case '"':
    return _make_string(scanner);
  case '\'':
    return make_char(scanner);
  }

  return _error_token(*scanner, "Unexpected character.");
}

void scanner_init(struct scanner *scanner, const char *source) {
  *scanner = (struct scanner){.start = source, .current = source, .line = 1};
}
