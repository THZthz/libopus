// CodeMirror, copyright (c) by Marijn Haverbeke and others
// Distributed under an MIT license: https://codemirror.net/LICENSE

(function(mod) {
  if (typeof exports == "object" && typeof module == "object") // CommonJS
    mod(require("../../lib/codemirror"), require("../xml/xml"), require("../meta"));
  else if (typeof define == "function" && define.amd) // AMD
    define(["../../lib/codemirror", "../xml/xml", "../meta"], mod);
  else // Plain browser env
    mod(CodeMirror);
})(function(CodeMirror) {
"use strict";

CodeMirror.defineMode("markdown", function(cmCfg, modeCfg) {

  var htmlMode = CodeMirror.getMode(cmCfg, "text/html");
  var htmlModeMissing = htmlMode.name == "null"

  function getMode(name) {
    if (CodeMirror.findModeByName) {
      var found = CodeMirror.findModeByName(name);
      if (found) name = found.mime || found.mimes[0];
    }
    var mode = CodeMirror.getMode(cmCfg, name);
    return mode.name == "null" ? null : mode;
  }

  // Should characters that affect highlighting be highlighted separate?
  // Does not include characters that will be output (such as `1.` and `-` for lists)
  if (modeCfg.highlightFormatting === undefined)
    modeCfg.highlightFormatting = false;

  // Maximum number of nested blockquotes. Set to 0 for infinite nesting.
  // Excess `>` will emit `error` token.
  if (modeCfg.maxBlockquoteDepth === undefined)
    modeCfg.maxBlockquoteDepth = 0;

  // Turn on task lists? ("- [ ] " and "- [x] ")
  if (modeCfg.taskLists === undefined) modeCfg.taskLists = false;

  // Turn on strikethrough syntax
  if (modeCfg.strikethrough === undefined)
    modeCfg.strikethrough = false;

  if (modeCfg.emoji === undefined)
    modeCfg.emoji = false;

  if (modeCfg.fencedCodeBlockHighlighting === undefined)
    modeCfg.fencedCodeBlockHighlighting = true;

  if (modeCfg.fencedCodeBlockDefaultMode === undefined)
    modeCfg.fencedCodeBlockDefaultMode = 'text/plain';

  if (modeCfg.xml === undefined)
    modeCfg.xml = true;

  // Allow token types to be overridden by user-provided token types.
  if (modeCfg.tokenTypeOverrides === undefined)
    modeCfg.tokenTypeOverrides = {};

  var tokenTypes = {
    header: "header",
    code: "comment",
    quote: "quote",
    list1: "variable-2",
    list2: "variable-3",
    list3: "keyword",
    hr: "hr",
    image: "image",
    imageAltText: "image-alt-text",
    imageMarker: "image-marker",
    formatting: "formatting",
    linkInline: "link",
    linkEmail: "link",
    linkText: "link",
    linkHref: "string",
    em: "em",
    strong: "strong",
    strikethrough: "strikethrough",
    emoji: "builtin"
  };

  for (var tokenType in tokenTypes) {
    if (tokenTypes.hasOwnProperty(tokenType) && modeCfg.tokenTypeOverrides[tokenType]) {
      tokenTypes[tokenType] = modeCfg.tokenTypeOverrides[tokenType];
    }
  }

  var hrRE = /^([*\-_])(?:\s*\1){2,}\s*$/
  ,   listRE = /^(?:[*\-+]|^[0-9]+([.)]))\s+/
  ,   taskListRE = /^\[(x| )\](?=\s)/i // Must follow listRE
  ,   atxHeaderRE = modeCfg.allowAtxHeaderWithoutSpace ? /^(#+)/ : /^(#+)(?: |$)/
  ,   setextHeaderRE = /^ {0,3}(?:\={1,}|-{2,})\s*$/
  ,   textRE = /^[^#!\[\]*_\\<>` "'(~:]+/
  ,   fencedCodeRE = /^(~~~+|```+)[ \t]*([\w\/+#-]*)[^\n`]*$/
  ,   linkDefRE = /^\s*\[[^\]]+?\]:.*$/ // naive link-definition
  ,   punctuation = /[!"#$%&'()*+,\-.\/:;<=>?@\[\\\]^_`{|}~\xA1\xA7\xAB\xB6\xB7\xBB\xBF\u037E\u0387\u055A-\u055F\u0589\u058A\u05BE\u05C0\u05C3\u05C6\u05F3\u05F4\u0609\u060A\u060C\u060D\u061B\u061E\u061F\u066A-\u066D\u06D4\u0700-\u070D\u07F7-\u07F9\u0830-\u083E\u085E\u0964\u0965\u0970\u0AF0\u0DF4\u0E4F\u0E5A\u0E5B\u0F04-\u0F12\u0F14\u0F3A-\u0F3D\u0F85\u0FD0-\u0FD4\u0FD9\u0FDA\u104A-\u104F\u10FB\u1360-\u1368\u1400\u166D\u166E\u169B\u169C\u16EB-\u16ED\u1735\u1736\u17D4-\u17D6\u17D8-\u17DA\u1800-\u180A\u1944\u1945\u1A1E\u1A1F\u1AA0-\u1AA6\u1AA8-\u1AAD\u1B5A-\u1B60\u1BFC-\u1BFF\u1C3B-\u1C3F\u1C7E\u1C7F\u1CC0-\u1CC7\u1CD3\u2010-\u2027\u2030-\u2043\u2045-\u2051\u2053-\u205E\u207D\u207E\u208D\u208E\u2308-\u230B\u2329\u232A\u2768-\u2775\u27C5\u27C6\u27E6-\u27EF\u2983-\u2998\u29D8-\u29DB\u29FC\u29FD\u2CF9-\u2CFC\u2CFE\u2CFF\u2D70\u2E00-\u2E2E\u2E30-\u2E42\u3001-\u3003\u3008-\u3011\u3014-\u301F\u3030\u303D\u30A0\u30FB\uA4FE\uA4FF\uA60D-\uA60F\uA673\uA67E\uA6F2-\uA6F7\uA874-\uA877\uA8CE\uA8CF\uA8F8-\uA8FA\uA8FC\uA92E\uA92F\uA95F\uA9C1-\uA9CD\uA9DE\uA9DF\uAA5C-\uAA5F\uAADE\uAADF\uAAF0\uAAF1\uABEB\uFD3E\uFD3F\uFE10-\uFE19\uFE30-\uFE52\uFE54-\uFE61\uFE63\uFE68\uFE6A\uFE6B\uFF01-\uFF03\uFF05-\uFF0A\uFF0C-\uFF0F\uFF1A\uFF1B\uFF1F\uFF20\uFF3B-\uFF3D\uFF3F\uFF5B\uFF5D\uFF5F-\uFF65]|\uD800[\uDD00-\uDD02\uDF9F\uDFD0]|\uD801\uDD6F|\uD802[\uDC57\uDD1F\uDD3F\uDE50-\uDE58\uDE7F\uDEF0-\uDEF6\uDF39-\uDF3F\uDF99-\uDF9C]|\uD804[\uDC47-\uDC4D\uDCBB\uDCBC\uDCBE-\uDCC1\uDD40-\uDD43\uDD74\uDD75\uDDC5-\uDDC9\uDDCD\uDDDB\uDDDD-\uDDDF\uDE38-\uDE3D\uDEA9]|\uD805[\uDCC6\uDDC1-\uDDD7\uDE41-\uDE43\uDF3C-\uDF3E]|\uD809[\uDC70-\uDC74]|\uD81A[\uDE6E\uDE6F\uDEF5\uDF37-\uDF3B\uDF44]|\uD82F\uDC9F|\uD836[\uDE87-\uDE8B]/
  ,   expandedTab = "    " // CommonMark specifies tab as 4 spaces

  function switchInline(stream, state__, f) {
    state__.f = state__.inline = f;
    return f(stream, state__);
  }

  function switchBlock(stream, state__, f) {
    state__.f = state__.block = f;
    return f(stream, state__);
  }

  function lineIsEmpty(line) {
    return !line || !/\S/.test(line.string)
  }

  // Blocks

  function blankLine(state__) {
    // Reset linkTitle state
    state__.linkTitle = false;
    state__.linkHref = false;
    state__.linkText = false;
    // Reset EM state
    state__.em = false;
    // Reset STRONG state
    state__.strong = false;
    // Reset strikethrough state
    state__.strikethrough = false;
    // Reset state.quote
    state__.quote = 0;
    // Reset state.indentedCode
    state__.indentedCode = false;
    if (state__.f == htmlBlock) {
      var exit = htmlModeMissing
      if (!exit) {
        var inner = CodeMirror.innerMode(htmlMode, state__.htmlState)
        exit = inner.mode.name == "xml" && inner.state__.tagStart === null &&
          (!inner.state__.context && inner.state__.tokenize.isInText)
      }
      if (exit) {
        state__.f = inlineNormal;
        state__.block = blockNormal;
        state__.htmlState = null;
      }
    }
    // Reset state.trailingSpace
    state__.trailingSpace = 0;
    state__.trailingSpaceNewLine = false;
    // Mark this line as blank
    state__.prevLine = state__.thisLine
    state__.thisLine = {stream: null}
    return null;
  }

  function blockNormal(stream, state__) {
    var firstTokenOnLine = stream.column() === state__.indentation;
    var prevLineLineIsEmpty = lineIsEmpty(state__.prevLine.stream);
    var prevLineIsIndentedCode = state__.indentedCode;
    var prevLineIsHr = state__.prevLine.hr;
    var prevLineIsList = state__.list !== false;
    var maxNonCodeIndentation = (state__.listStack[state__.listStack.length - 1] || 0) + 3;

    state__.indentedCode = false;

    var lineIndentation = state__.indentation;
    // compute once per line (on first token)
    if (state__.indentationDiff === null) {
      state__.indentationDiff = state__.indentation;
      if (prevLineIsList) {
        state__.list = null;
        // While this list item's marker's indentation is less than the deepest
        //  list item's content's indentation,pop the deepest list item
        //  indentation off the stack, and update block indentation state
        while (lineIndentation < state__.listStack[state__.listStack.length - 1]) {
          state__.listStack.pop();
          if (state__.listStack.length) {
            state__.indentation = state__.listStack[state__.listStack.length - 1];
          // less than the first list's indent -> the line is no longer a list
          } else {
            state__.list = false;
          }
        }
        if (state__.list !== false) {
          state__.indentationDiff = lineIndentation - state__.listStack[state__.listStack.length - 1]
        }
      }
    }

    // not comprehensive (currently only for setext detection purposes)
    var allowsInlineContinuation = (
        !prevLineLineIsEmpty && !prevLineIsHr && !state__.prevLine.header &&
        (!prevLineIsList || !prevLineIsIndentedCode) &&
        !state__.prevLine.fencedCodeEnd
    );

    var isHr = (state__.list === false || prevLineIsHr || prevLineLineIsEmpty) &&
      state__.indentation <= maxNonCodeIndentation && stream.match(hrRE);

    var match = null;
    if (state__.indentationDiff >= 4 && (prevLineIsIndentedCode || state__.prevLine.fencedCodeEnd ||
         state__.prevLine.header || prevLineLineIsEmpty)) {
      stream.skipToEnd();
      state__.indentedCode = true;
      return tokenTypes.code;
    } else if (stream.eatSpace()) {
      return null;
    } else if (firstTokenOnLine && state__.indentation <= maxNonCodeIndentation && (match = stream.match(atxHeaderRE)) && match[1].length <= 6) {
      state__.quote = 0;
      state__.header = match[1].length;
      state__.thisLine.header = true;
      if (modeCfg.highlightFormatting) state__.formatting = "header";
      state__.f = state__.inline;
      return getType(state__);
    } else if (state__.indentation <= maxNonCodeIndentation && stream.eat('>')) {
      state__.quote = firstTokenOnLine ? 1 : state__.quote + 1;
      if (modeCfg.highlightFormatting) state__.formatting = "quote";
      stream.eatSpace();
      return getType(state__);
    } else if (!isHr && !state__.setext && firstTokenOnLine && state__.indentation <= maxNonCodeIndentation && (match = stream.match(listRE))) {
      var listType = match[1] ? "ol" : "ul";

      state__.indentation = lineIndentation + stream.current().length;
      state__.list = true;
      state__.quote = 0;

      // Add this list item's content's indentation to the stack
      state__.listStack.push(state__.indentation);
      // Reset inline styles which shouldn't propagate across list items
      state__.em = false;
      state__.strong = false;
      state__.code = false;
      state__.strikethrough = false;

      if (modeCfg.taskLists && stream.match(taskListRE, false)) {
        state__.taskList = true;
      }
      state__.f = state__.inline;
      if (modeCfg.highlightFormatting) state__.formatting = ["list", "list-" + listType];
      return getType(state__);
    } else if (firstTokenOnLine && state__.indentation <= maxNonCodeIndentation && (match = stream.match(fencedCodeRE, true))) {
      state__.quote = 0;
      state__.fencedEndRE = new RegExp(match[1] + "+ *$");
      // try switching mode
      state__.localMode = modeCfg.fencedCodeBlockHighlighting && getMode(match[2] || modeCfg.fencedCodeBlockDefaultMode );
      if (state__.localMode) state__.localState = CodeMirror.startState(state__.localMode);
      state__.f = state__.block = local;
      if (modeCfg.highlightFormatting) state__.formatting = "code-block";
      state__.code = -1
      return getType(state__);
    // SETEXT has lowest block-scope precedence after HR, so check it after
    //  the others (code, blockquote, list...)
    } else if (
      // if setext set, indicates line after ---/===
      state__.setext || (
        // line before ---/===
        (!allowsInlineContinuation || !prevLineIsList) && !state__.quote && state__.list === false &&
        !state__.code && !isHr && !linkDefRE.test(stream.string) &&
        (match = stream.lookAhead(1)) && (match = match.match(setextHeaderRE))
      )
    ) {
      if ( !state__.setext ) {
        state__.header = match[0].charAt(0) == '=' ? 1 : 2;
        state__.setext = state__.header;
      } else {
        state__.header = state__.setext;
        // has no effect on type so we can reset it now
        state__.setext = 0;
        stream.skipToEnd();
        if (modeCfg.highlightFormatting) state__.formatting = "header";
      }
      state__.thisLine.header = true;
      state__.f = state__.inline;
      return getType(state__);
    } else if (isHr) {
      stream.skipToEnd();
      state__.hr = true;
      state__.thisLine.hr = true;
      return tokenTypes.hr;
    } else if (stream.peek() === '[') {
      return switchInline(stream, state__, footnoteLink);
    }

    return switchInline(stream, state__, state__.inline);
  }

  function htmlBlock(stream, state__) {
    var style = htmlMode.token(stream, state__.htmlState);
    if (!htmlModeMissing) {
      var inner = CodeMirror.innerMode(htmlMode, state__.htmlState)
      if ((inner.mode.name == "xml" && inner.state__.tagStart === null &&
           (!inner.state__.context && inner.state__.tokenize.isInText)) ||
          (state__.md_inside && stream.current().indexOf(">") > -1)) {
        state__.f = inlineNormal;
        state__.block = blockNormal;
        state__.htmlState = null;
      }
    }
    return style;
  }

  function local(stream, state__) {
    var currListInd = state__.listStack[state__.listStack.length - 1] || 0;
    var hasExitedList = state__.indentation < currListInd;
    var maxFencedEndInd = currListInd + 3;
    if (state__.fencedEndRE && state__.indentation <= maxFencedEndInd && (hasExitedList || stream.match(state__.fencedEndRE))) {
      if (modeCfg.highlightFormatting) state__.formatting = "code-block";
      var returnType;
      if (!hasExitedList) returnType = getType(state__)
      state__.localMode = state__.localState = null;
      state__.block = blockNormal;
      state__.f = inlineNormal;
      state__.fencedEndRE = null;
      state__.code = 0
      state__.thisLine.fencedCodeEnd = true;
      if (hasExitedList) return switchBlock(stream, state__, state__.block);
      return returnType;
    } else if (state__.localMode) {
      return state__.localMode.token(stream, state__.localState);
    } else {
      stream.skipToEnd();
      return tokenTypes.code;
    }
  }

  // Inline
  function getType(state__) {
    var styles = [];

    if (state__.formatting) {
      styles.push(tokenTypes.formatting);

      if (typeof state__.formatting === "string") state__.formatting = [state__.formatting];

      for (var i = 0; i < state__.formatting.length; i++) {
        styles.push(tokenTypes.formatting + "-" + state__.formatting[i]);

        if (state__.formatting[i] === "header") {
          styles.push(tokenTypes.formatting + "-" + state__.formatting[i] + "-" + state__.header);
        }

        // Add `formatting-quote` and `formatting-quote-#` for blockquotes
        // Add `error` instead if the maximum blockquote nesting depth is passed
        if (state__.formatting[i] === "quote") {
          if (!modeCfg.maxBlockquoteDepth || modeCfg.maxBlockquoteDepth >= state__.quote) {
            styles.push(tokenTypes.formatting + "-" + state__.formatting[i] + "-" + state__.quote);
          } else {
            styles.push("error");
          }
        }
      }
    }

    if (state__.taskOpen) {
      styles.push("meta");
      return styles.length ? styles.join(' ') : null;
    }
    if (state__.taskClosed) {
      styles.push("property");
      return styles.length ? styles.join(' ') : null;
    }

    if (state__.linkHref) {
      styles.push(tokenTypes.linkHref, "url");
    } else { // Only apply inline styles to non-url text
      if (state__.strong) { styles.push(tokenTypes.strong); }
      if (state__.em) { styles.push(tokenTypes.em); }
      if (state__.strikethrough) { styles.push(tokenTypes.strikethrough); }
      if (state__.emoji) { styles.push(tokenTypes.emoji); }
      if (state__.linkText) { styles.push(tokenTypes.linkText); }
      if (state__.code) { styles.push(tokenTypes.code); }
      if (state__.image) { styles.push(tokenTypes.image); }
      if (state__.imageAltText) { styles.push(tokenTypes.imageAltText, "link"); }
      if (state__.imageMarker) { styles.push(tokenTypes.imageMarker); }
    }

    if (state__.header) { styles.push(tokenTypes.header, tokenTypes.header + "-" + state__.header); }

    if (state__.quote) {
      styles.push(tokenTypes.quote);

      // Add `quote-#` where the maximum for `#` is modeCfg.maxBlockquoteDepth
      if (!modeCfg.maxBlockquoteDepth || modeCfg.maxBlockquoteDepth >= state__.quote) {
        styles.push(tokenTypes.quote + "-" + state__.quote);
      } else {
        styles.push(tokenTypes.quote + "-" + modeCfg.maxBlockquoteDepth);
      }
    }

    if (state__.list !== false) {
      var listMod = (state__.listStack.length - 1) % 3;
      if (!listMod) {
        styles.push(tokenTypes.list1);
      } else if (listMod === 1) {
        styles.push(tokenTypes.list2);
      } else {
        styles.push(tokenTypes.list3);
      }
    }

    if (state__.trailingSpaceNewLine) {
      styles.push("trailing-space-new-line");
    } else if (state__.trailingSpace) {
      styles.push("trailing-space-" + (state__.trailingSpace % 2 ? "a" : "b"));
    }

    return styles.length ? styles.join(' ') : null;
  }

  function handleText(stream, state__) {
    if (stream.match(textRE, true)) {
      return getType(state__);
    }
    return undefined;
  }

  function inlineNormal(stream, state__) {
    var style = state__.text(stream, state__);
    if (typeof style !== 'undefined')
      return style;

    if (state__.list) { // List marker (*, +, -, 1., etc)
      state__.list = null;
      return getType(state__);
    }

    if (state__.taskList) {
      var taskOpen = stream.match(taskListRE, true)[1] === " ";
      if (taskOpen) state__.taskOpen = true;
      else state__.taskClosed = true;
      if (modeCfg.highlightFormatting) state__.formatting = "task";
      state__.taskList = false;
      return getType(state__);
    }

    state__.taskOpen = false;
    state__.taskClosed = false;

    if (state__.header && stream.match(/^#+$/, true)) {
      if (modeCfg.highlightFormatting) state__.formatting = "header";
      return getType(state__);
    }

    var ch = stream.next();

    // Matches link titles present on next line
    if (state__.linkTitle) {
      state__.linkTitle = false;
      var matchCh = ch;
      if (ch === '(') {
        matchCh = ')';
      }
      matchCh = (matchCh+'').replace(/([.?*+^\[\]\\(){}|-])/g, "\\$1");
      var regex = '^\\s*(?:[^' + matchCh + '\\\\]+|\\\\\\\\|\\\\.)' + matchCh;
      if (stream.match(new RegExp(regex), true)) {
        return tokenTypes.linkHref;
      }
    }

    // If this block is changed, it may need to be updated in GFM mode
    if (ch === '`') {
      var previousFormatting = state__.formatting;
      if (modeCfg.highlightFormatting) state__.formatting = "code";
      stream.eatWhile('`');
      var count = stream.current().length
      if (state__.code == 0 && (!state__.quote || count == 1)) {
        state__.code = count
        return getType(state__)
      } else if (count == state__.code) { // Must be exact
        var t = getType(state__)
        state__.code = 0
        return t
      } else {
        state__.formatting = previousFormatting
        return getType(state__)
      }
    } else if (state__.code) {
      return getType(state__);
    }

    if (ch === '\\') {
      stream.next();
      if (modeCfg.highlightFormatting) {
        var type = getType(state__);
        var formattingEscape = tokenTypes.formatting + "-escape";
        return type ? type + " " + formattingEscape : formattingEscape;
      }
    }

    if (ch === '!' && stream.match(/\[[^\]]*\] ?(?:\(|\[)/, false)) {
      state__.imageMarker = true;
      state__.image = true;
      if (modeCfg.highlightFormatting) state__.formatting = "image";
      return getType(state__);
    }

    if (ch === '[' && state__.imageMarker && stream.match(/[^\]]*\](\(.*?\)| ?\[.*?\])/, false)) {
      state__.imageMarker = false;
      state__.imageAltText = true
      if (modeCfg.highlightFormatting) state__.formatting = "image";
      return getType(state__);
    }

    if (ch === ']' && state__.imageAltText) {
      if (modeCfg.highlightFormatting) state__.formatting = "image";
      var type = getType(state__);
      state__.imageAltText = false;
      state__.image = false;
      state__.inline = state__.f = linkHref;
      return type;
    }

    if (ch === '[' && !state__.image) {
      if (state__.linkText && stream.match(/^.*?\]/)) return getType(state__)
      state__.linkText = true;
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      return getType(state__);
    }

    if (ch === ']' && state__.linkText) {
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      var type = getType(state__);
      state__.linkText = false;
      state__.inline = state__.f = stream.match(/\(.*?\)| ?\[.*?\]/, false) ? linkHref : inlineNormal
      return type;
    }

    if (ch === '<' && stream.match(/^(https?|ftps?):\/\/(?:[^\\>]|\\.)+>/, false)) {
      state__.f = state__.inline = linkInline;
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      var type = getType(state__);
      if (type){
        type += " ";
      } else {
        type = "";
      }
      return type + tokenTypes.linkInline;
    }

    if (ch === '<' && stream.match(/^[^> \\]+@(?:[^\\>]|\\.)+>/, false)) {
      state__.f = state__.inline = linkInline;
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      var type = getType(state__);
      if (type){
        type += " ";
      } else {
        type = "";
      }
      return type + tokenTypes.linkEmail;
    }

    if (modeCfg.xml && ch === '<' && stream.match(/^(!--|\?|!\[CDATA\[|[a-z][a-z0-9-]*(?:\s+[a-z_:.\-]+(?:\s*=\s*[^>]+)?)*\s*(?:>|$))/i, false)) {
      var end = stream.string.indexOf(">", stream.pos);
      if (end != -1) {
        var atts = stream.string.substring(stream.start, end);
        if (/markdown\s*=\s*('|"){0,1}1('|"){0,1}/.test(atts)) state__.md_inside = true;
      }
      stream.backUp(1);
      state__.htmlState = CodeMirror.startState(htmlMode);
      return switchBlock(stream, state__, htmlBlock);
    }

    if (modeCfg.xml && ch === '<' && stream.match(/^\/\w*?>/)) {
      state__.md_inside = false;
      return "tag";
    } else if (ch === "*" || ch === "_") {
      var len = 1, before = stream.pos == 1 ? " " : stream.string.charAt(stream.pos - 2)
      while (len < 3 && stream.eat(ch)) len++
      var after = stream.peek() || " "
      // See http://spec.commonmark.org/0.27/#emphasis-and-strong-emphasis
      var leftFlanking = !/\s/.test(after) && (!punctuation.test(after) || /\s/.test(before) || punctuation.test(before))
      var rightFlanking = !/\s/.test(before) && (!punctuation.test(before) || /\s/.test(after) || punctuation.test(after))
      var setEm = null, setStrong = null
      if (len % 2) { // Em
        if (!state__.em && leftFlanking && (ch === "*" || !rightFlanking || punctuation.test(before)))
          setEm = true
        else if (state__.em == ch && rightFlanking && (ch === "*" || !leftFlanking || punctuation.test(after)))
          setEm = false
      }
      if (len > 1) { // Strong
        if (!state__.strong && leftFlanking && (ch === "*" || !rightFlanking || punctuation.test(before)))
          setStrong = true
        else if (state__.strong == ch && rightFlanking && (ch === "*" || !leftFlanking || punctuation.test(after)))
          setStrong = false
      }
      if (setStrong != null || setEm != null) {
        if (modeCfg.highlightFormatting) state__.formatting = setEm == null ? "strong" : setStrong == null ? "em" : "strong em"
        if (setEm === true) state__.em = ch
        if (setStrong === true) state__.strong = ch
        var t = getType(state__)
        if (setEm === false) state__.em = false
        if (setStrong === false) state__.strong = false
        return t
      }
    } else if (ch === ' ') {
      if (stream.eat('*') || stream.eat('_')) { // Probably surrounded by spaces
        if (stream.peek() === ' ') { // Surrounded by spaces, ignore
          return getType(state__);
        } else { // Not surrounded by spaces, back up pointer
          stream.backUp(1);
        }
      }
    }

    if (modeCfg.strikethrough) {
      if (ch === '~' && stream.eatWhile(ch)) {
        if (state__.strikethrough) {// Remove strikethrough
          if (modeCfg.highlightFormatting) state__.formatting = "strikethrough";
          var t = getType(state__);
          state__.strikethrough = false;
          return t;
        } else if (stream.match(/^[^\s]/, false)) {// Add strikethrough
          state__.strikethrough = true;
          if (modeCfg.highlightFormatting) state__.formatting = "strikethrough";
          return getType(state__);
        }
      } else if (ch === ' ') {
        if (stream.match('~~', true)) { // Probably surrounded by space
          if (stream.peek() === ' ') { // Surrounded by spaces, ignore
            return getType(state__);
          } else { // Not surrounded by spaces, back up pointer
            stream.backUp(2);
          }
        }
      }
    }

    if (modeCfg.emoji && ch === ":" && stream.match(/^(?:[a-z_\d+][a-z_\d+-]*|\-[a-z_\d+][a-z_\d+-]*):/)) {
      state__.emoji = true;
      if (modeCfg.highlightFormatting) state__.formatting = "emoji";
      var retType = getType(state__);
      state__.emoji = false;
      return retType;
    }

    if (ch === ' ') {
      if (stream.match(/^ +$/, false)) {
        state__.trailingSpace++;
      } else if (state__.trailingSpace) {
        state__.trailingSpaceNewLine = true;
      }
    }

    return getType(state__);
  }

  function linkInline(stream, state__) {
    var ch = stream.next();

    if (ch === ">") {
      state__.f = state__.inline = inlineNormal;
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      var type = getType(state__);
      if (type){
        type += " ";
      } else {
        type = "";
      }
      return type + tokenTypes.linkInline;
    }

    stream.match(/^[^>]+/, true);

    return tokenTypes.linkInline;
  }

  function linkHref(stream, state__) {
    // Check if space, and return NULL if so (to avoid marking the space)
    if(stream.eatSpace()){
      return null;
    }
    var ch = stream.next();
    if (ch === '(' || ch === '[') {
      state__.f = state__.inline = getLinkHrefInside(ch === "(" ? ")" : "]");
      if (modeCfg.highlightFormatting) state__.formatting = "link-string";
      state__.linkHref = true;
      return getType(state__);
    }
    return 'error';
  }

  var linkRE = {
    ")": /^(?:[^\\\(\)]|\\.|\((?:[^\\\(\)]|\\.)*\))*?(?=\))/,
    "]": /^(?:[^\\\[\]]|\\.|\[(?:[^\\\[\]]|\\.)*\])*?(?=\])/
  }

  function getLinkHrefInside(endChar) {
    return function(stream, state__) {
      var ch = stream.next();

      if (ch === endChar) {
        state__.f = state__.inline = inlineNormal;
        if (modeCfg.highlightFormatting) state__.formatting = "link-string";
        var returnState = getType(state__);
        state__.linkHref = false;
        return returnState;
      }

      stream.match(linkRE[endChar])
      state__.linkHref = true;
      return getType(state__);
    };
  }

  function footnoteLink(stream, state__) {
    if (stream.match(/^([^\]\\]|\\.)*\]:/, false)) {
      state__.f = footnoteLinkInside;
      stream.next(); // Consume [
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      state__.linkText = true;
      return getType(state__);
    }
    return switchInline(stream, state__, inlineNormal);
  }

  function footnoteLinkInside(stream, state__) {
    if (stream.match(']:', true)) {
      state__.f = state__.inline = footnoteUrl;
      if (modeCfg.highlightFormatting) state__.formatting = "link";
      var returnType = getType(state__);
      state__.linkText = false;
      return returnType;
    }

    stream.match(/^([^\]\\]|\\.)+/, true);

    return tokenTypes.linkText;
  }

  function footnoteUrl(stream, state__) {
    // Check if space, and return NULL if so (to avoid marking the space)
    if(stream.eatSpace()){
      return null;
    }
    // Match URL
    stream.match(/^[^\s]+/, true);
    // Check for link title
    if (stream.peek() === undefined) { // End of line, set flag to check next line
      state__.linkTitle = true;
    } else { // More content on line, check if link title
      stream.match(/^(?:\s+(?:"(?:[^"\\]|\\.)+"|'(?:[^'\\]|\\.)+'|\((?:[^)\\]|\\.)+\)))?/, true);
    }
    state__.f = state__.inline = inlineNormal;
    return tokenTypes.linkHref + " url";
  }

  var mode = {
    startState: function() {
      return {
        f: blockNormal,

        prevLine: {stream: null},
        thisLine: {stream: null},

        block: blockNormal,
        htmlState: null,
        indentation: 0,

        inline: inlineNormal,
        text: handleText,

        formatting: false,
        linkText: false,
        linkHref: false,
        linkTitle: false,
        code: 0,
        em: false,
        strong: false,
        header: 0,
        setext: 0,
        hr: false,
        taskList: false,
        list: false,
        listStack: [],
        quote: 0,
        trailingSpace: 0,
        trailingSpaceNewLine: false,
        strikethrough: false,
        emoji: false,
        fencedEndRE: null
      };
    },

    copyState: function(s) {
      return {
        f: s.f,

        prevLine: s.prevLine,
        thisLine: s.thisLine,

        block: s.block,
        htmlState: s.htmlState && CodeMirror.copyState(htmlMode, s.htmlState),
        indentation: s.indentation,

        localMode: s.localMode,
        localState: s.localMode ? CodeMirror.copyState(s.localMode, s.localState) : null,

        inline: s.inline,
        text: s.text,
        formatting: false,
        linkText: s.linkText,
        linkTitle: s.linkTitle,
        linkHref: s.linkHref,
        code: s.code,
        em: s.em,
        strong: s.strong,
        strikethrough: s.strikethrough,
        emoji: s.emoji,
        header: s.header,
        setext: s.setext,
        hr: s.hr,
        taskList: s.taskList,
        list: s.list,
        listStack: s.listStack.slice(0),
        quote: s.quote,
        indentedCode: s.indentedCode,
        trailingSpace: s.trailingSpace,
        trailingSpaceNewLine: s.trailingSpaceNewLine,
        md_inside: s.md_inside,
        fencedEndRE: s.fencedEndRE
      };
    },

    token: function(stream, state__) {

      // Reset state.formatting
      state__.formatting = false;

      if (stream != state__.thisLine.stream) {
        state__.header = 0;
        state__.hr = false;

        if (stream.match(/^\s*$/, true)) {
          blankLine(state__);
          return null;
        }

        state__.prevLine = state__.thisLine
        state__.thisLine = {stream: stream}

        // Reset state.taskList
        state__.taskList = false;

        // Reset state.trailingSpace
        state__.trailingSpace = 0;
        state__.trailingSpaceNewLine = false;

        if (!state__.localState) {
          state__.f = state__.block;
          if (state__.f != htmlBlock) {
            var indentation = stream.match(/^\s*/, true)[0].replace(/\t/g, expandedTab).length;
            state__.indentation = indentation;
            state__.indentationDiff = null;
            if (indentation > 0) return null;
          }
        }
      }
      return state__.f(stream, state__);
    },

    innerMode: function(state__) {
      if (state__.block == htmlBlock) return {state__: state__.htmlState, mode: htmlMode};
      if (state__.localState) return {state__: state__.localState, mode: state__.localMode};
      return {state__: state__, mode: mode};
    },

    indent: function(state__, textAfter, line) {
      if (state__.block == htmlBlock && htmlMode.indent) return htmlMode.indent(state__.htmlState, textAfter, line)
      if (state__.localState && state__.localMode.indent) return state__.localMode.indent(state__.localState, textAfter, line)
      return CodeMirror.Pass
    },

    blankLine: blankLine,

    getType: getType,

    blockCommentStart: "<!--",
    blockCommentEnd: "-->",
    closeBrackets: "()[]{}''\"\"``",
    fold: "markdown"
  };
  return mode;
}, "xml");

CodeMirror.defineMIME("text/markdown", "markdown");

CodeMirror.defineMIME("text/x-markdown", "markdown");

});
