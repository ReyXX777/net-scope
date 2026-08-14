// hexview.h
#ifndef HEXVIEW_H
#define HEXVIEW_H

#include <QAbstractScrollArea>
#include <QByteArray>
#include <QPoint>

class HexView : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit HexView(QWidget *parent = nullptr);
    void setData(const QByteArray &data);
    void setHighlight(int offset, int length);
    void clearHighlight();

protected:
    void paintEvent(QPaintEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    QByteArray m_data;
    QPoint m_highlight; // x = offset, y = length
    int m_bytesPerLine;
    int m_lineHeight;
    int m_hexAreaWidth;
    int m_asciiAreaX;
    
    void updateScrollBars();
};

#endif // HEXVIEW_H