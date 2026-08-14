// hexview.cpp
#include "hexview.h"
#include <QPainter>
#include <QScrollBar>
#include <QFontMetrics>

HexView::HexView(QWidget *parent) : QAbstractScrollArea(parent), m_highlight(-1, 0) {
    m_bytesPerLine = 16;
    QFont font("Courier New", 10);
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    
    QFontMetrics fm(font);
    m_lineHeight = fm.height();
    m_hexAreaWidth = m_bytesPerLine * 3 * fm.averageCharWidth();
    m_asciiAreaX = m_hexAreaWidth + 20;
    
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void HexView::setData(const QByteArray &data) {
    m_data = data;
    m_highlight = QPoint(-1, 0);
    updateScrollBars();
    viewport()->update();
}

void HexView::setHighlight(int offset, int length) {
    m_highlight = QPoint(offset, length);
    viewport()->update();
}

void HexView::clearHighlight() {
    m_highlight = QPoint(-1, 0);
    viewport()->update();
}

void HexView::updateScrollBars() {
    int totalLines = (m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine;
    verticalScrollBar()->setRange(0, qMax(0, totalLines - viewport()->height() / m_lineHeight));
    verticalScrollBar()->setPageStep(viewport()->height() / m_lineHeight);
}

void HexView::scrollContentsBy(int, int) {
    viewport()->update();
}

void HexView::paintEvent(QPaintEvent *) {
    QPainter painter(viewport());
    painter.setFont(font());
    QFontMetrics fm(font());
    
    int scrollY = verticalScrollBar()->value();
    int startLine = scrollY;
    int endLine = qMin(startLine + (viewport()->height() / m_lineHeight) + 1, 
                       (m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine);
    
    painter.fillRect(viewport()->rect(), Qt::white);
    
    for (int line = startLine; line < endLine; ++line) {
        int y = (line - startLine) * m_lineHeight;
        int offset = line * m_bytesPerLine;
        
        // Draw address
        painter.setPen(Qt::darkGray);
        painter.drawText(0, y + fm.ascent(), QString("%1").arg(offset, 4, 16, QChar('0')));
        
        // Draw hex bytes
        for (int col = 0; col < m_bytesPerLine; ++col) {
            int byteIdx = offset + col;
            if (byteIdx >= m_data.size()) break;
            
            int x = 50 + (col * 3 * fm.averageCharWidth());
            unsigned char byte = m_data[byteIdx];
            
            bool isHighlighted = (m_highlight.y() > 0 && 
                                  byteIdx >= m_highlight.x() && 
                                  byteIdx < m_highlight.x() + m_highlight.y());
            
            if (isHighlighted) {
                painter.fillRect(x, y, fm.averageCharWidth() * 2, m_lineHeight, QColor(0xFF, 0xFF, 0x00));
                painter.setPen(Qt::black);
            } else {
                painter.setPen(Qt::black);
            }
            
            painter.drawText(x, y + fm.ascent(), QString("%1").arg(byte, 2, 16, QChar('0')));
        }
        
        // Draw ASCII
        for (int col = 0; col < m_bytesPerLine; ++col) {
            int byteIdx = offset + col;
            if (byteIdx >= m_data.size()) break;
            
            int x = m_asciiAreaX + (col * fm.averageCharWidth());
            unsigned char byte = m_data[byteIdx];
            QChar ch = (byte >= 32 && byte < 127) ? QChar(byte) : '.';
            
            bool isHighlighted = (m_highlight.y() > 0 && 
                                  byteIdx >= m_highlight.x() && 
                                  byteIdx < m_highlight.x() + m_highlight.y());
            
            if (isHighlighted) {
                painter.fillRect(x, y, fm.averageCharWidth(), m_lineHeight, QColor(0xFF, 0xFF, 0x00));
                painter.setPen(Qt::black);
            } else {
                painter.setPen(Qt::darkGreen);
            }
            
            painter.drawText(x, y + fm.ascent(), ch);
        }
    }
}