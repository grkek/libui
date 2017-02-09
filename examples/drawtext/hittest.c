// 20 january 2017
#include "drawtext.h"

// TODO have a ligature
// TODO allow clicking on the end of a line to put the caret there
// TODO the hiding and showing does not work properly on GTK+

static const char *text =
	"Each of the glyphs an end user interacts with are called graphemes. "
	"If you enter a byte range in the text boxes below and click the button, you can see the blue box move to surround that byte range, as well as what the actual byte range necessary is. "
	// TODO rephrase this; I don't think this code will use those grapheme functions...
	"You'll also see the index of the first grapheme; uiAttributedString has facilities for converting between UTF-8 code points and grapheme indices. "
	"Additionally, click on the string to move the caret. Watch the status text at the bottom change too. "
	"That being said: "
	"\xC3\x93O\xCC\x81 (combining accents) "
	"A\xCC\xAA\xEF\xB8\xA0 (multiple combining characters) "
	"\xE2\x80\xAE#\xE2\x80\xAC (RTL glyph) "
	"\xF0\x9F\x92\xBB (non-BMP character) "
	"\xF0\x9F\x92\xBB\xCC\x80 (combined non-BMP character; may render strangely) "
	"";
static char fontFamily[] = "Helvetica";
static uiDrawFontDescriptor defaultFont = {
	.Family = fontFamily,
	.Size = 14,
	.Weight = uiDrawTextWeightNormal,
	.Italic = uiDrawTextItalicNormal,
	.Stretch = uiDrawTextStretchNormal,
};
static uiAttributedString *attrstr;

#define margins 10

static uiBox *panel;
static uiLabel *caretLabel;
static uiCheckbox *showLineBounds;

static int caretLine = -1;
static double caretX;

// TODO should be const?
static uiDrawBrush fillBrushes[4] = {
	{
		.Type = uiDrawBrushTypeSolid,
		.R = 1.0,
		.G = 0.0,
		.B = 0.0,
		.A = 0.5,
	},
	{
		.Type = uiDrawBrushTypeSolid,
		.R = 0.0,
		.G = 1.0,
		.B = 0.0,
		.A = 0.5,
	},
	{
		.Type = uiDrawBrushTypeSolid,
		.R = 0.0,
		.G = 0.0,
		.B = 1.0,
		.A = 0.5,
	},
	{
		.Type = uiDrawBrushTypeSolid,
		.R = 0.0,
		.G = 1.0,
		.B = 1.0,
		.A = 0.5,
	},
};

// TODO this should be const
static uiDrawStrokeParams strokeParams = {
	.Cap = uiDrawLineCapFlat,
	.Join = uiDrawLineJoinMiter,
	.Thickness = 2,
	.MiterLimit = uiDrawDefaultMiterLimit,
	.Dashes = NULL,
	.NumDashes = 0,
	.DashPhase = 0,
};

static void draw(uiAreaDrawParams *p)
{
	uiDrawPath *path;
	uiDrawTextLayout *layout;
	uiDrawTextLayoutLineMetrics m;
	uiDrawBrush brush;

	// only clip the text, not the guides
	uiDrawSave(p->Context);

	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathAddRectangle(path, margins, margins,
		p->AreaWidth - 2 * margins,
		p->AreaHeight - 2 * margins);
	uiDrawPathEnd(path);
	uiDrawClip(p->Context, path);
	uiDrawFreePath(path);

	layout = uiDrawNewTextLayout(attrstr,
		&defaultFont,
		p->AreaWidth - 2 * margins);
	uiDrawText(p->Context, layout, margins, margins);

	uiDrawRestore(p->Context);

	if (caretLine == -1) {
		caretLine = uiDrawTextLayoutNumLines(layout) - 1;
		caretX = uiDrawTextLayoutByteLocationInLine(layout,
			uiAttributedStringLen(attrstr),
			caretLine);
	}
	uiDrawTextLayoutLineGetMetrics(layout, caretLine, &m);
	path = uiDrawNewPath(uiDrawFillModeWinding);
	uiDrawPathNewFigure(path, margins + caretX, margins + m.Y);
	uiDrawPathLineTo(path, margins + caretX, margins + m.Y + m.Height);
	uiDrawPathEnd(path);
	brush.Type = uiDrawBrushTypeSolid;
	brush.R = 0.0;
	brush.G = 0.0;
	brush.B = 1.0;
	brush.A = 1.0;
	uiDrawStroke(p->Context, path, &brush, &strokeParams);
	uiDrawFreePath(path);

	if (uiCheckboxChecked(showLineBounds)) {
		int i, n;
		int fill = 0;

		n = uiDrawTextLayoutNumLines(layout);
		for (i = 0; i < n; i++) {
			uiDrawTextLayoutLineGetMetrics(layout, i, &m);
			path = uiDrawNewPath(uiDrawFillModeWinding);
			uiDrawPathAddRectangle(path, margins + m.X, margins + m.Y,
				m.Width, m.Height);
			uiDrawPathEnd(path);
			uiDrawFill(p->Context, path, fillBrushes + fill);
			uiDrawFreePath(path);
			fill = (fill + 1) % 4;
		}
	}

	uiDrawFreeTextLayout(layout);
}

static const char *positions[] = {
	[uiDrawTextLayoutHitTestPositionBefore] = "before",
	[uiDrawTextLayoutHitTestPositionInside] = "inside",
	[uiDrawTextLayoutHitTestPositionAfter] = "after",
};

static void mouse(uiAreaMouseEvent *e)
{
	uiDrawTextLayout *layout;
//	uiDrawTextLayoutHitTestResult res;
	size_t pos;
	char labelText[128];

	if (e->Down != 1)
		return;

	layout = uiDrawNewTextLayout(attrstr,
		&defaultFont,
		e->AreaWidth - 2 * margins);
	uiDrawTextLayoutHitTest(layout,
		e->X - margins, e->Y - margins,
//		&res);
		&pos, &caretLine);
	caretX = uiDrawTextLayoutByteLocationInLine(layout, pos, caretLine);
	uiDrawFreeTextLayout(layout);

	// urgh %zd is not supported by MSVC with sprintf()
	// TODO get that part in test/ about having no other option
	// TODO byte 1 is actually byte 684?!
	sprintf(labelText, "pos %d line %d x %g",// x position %s y position %s",
		(int) pos, caretLine, caretX);
/*		(int) (res.Pos), res.Line,
		positions[res.XPosition],
		positions[res.YPosition]);
*/	uiLabelSetText(caretLabel, labelText);

/*	cursorX = res.CaretX;
	cursorY = res.CaretY;
	cursorHeight = res.CaretHeight;
*/	redraw();
}

static struct example hitTestExample;

// TODO share?
static void checkboxChecked(uiCheckbox *c, void *data)
{
	redraw();
}

static uiCheckbox *newCheckbox(const char *text)
{
	uiCheckbox *c;

	c = uiNewCheckbox(text);
	uiCheckboxOnToggled(c, checkboxChecked, NULL);
	uiBoxAppend(panel, uiControl(c), 0);
	return c;
}

struct example *mkHitTestExample(void)
{
	panel = uiNewVerticalBox();
	caretLabel = uiNewLabel("Caret information is shown here");
	uiBoxAppend(panel, uiControl(caretLabel), 0);
	showLineBounds = newCheckbox("Show Line Bounds (for debugging metrics)");

	hitTestExample.name = "Hit-Testing and Grapheme Boundaries";
	hitTestExample.panel = uiControl(panel);
	hitTestExample.draw = draw;
	hitTestExample.mouse = mouse;

	attrstr = uiNewAttributedString(text);

	return &hitTestExample;
}
