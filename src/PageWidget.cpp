/*
 * This file is a part of QComicBook.
 *
 * Copyright (C) 2005-2009 Pawel Stolowski <stolowski@gmail.com>
 *
 * QComicBook is free software; you can redestribute it and/or modify it
 * under terms of GNU General Public License by Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY. See GPL for more details.
 */

#include "PageWidget.h"
#include "Page.h"
#include "View/PageViewBase.h"
#include "ComicBookSettings.h"
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include "ComicBookDebug.h"
#include "Job/PageRedrawJob.h"
#include "Job/ImageJobResult.h"
#include "Job/JobType.h"
#include <stdexcept>

using namespace QComicBook;

PageWidget::PageWidget(PageViewBase *parent, int w, int h, int pageNum, bool twoPages)
    : ComicImageWidget(parent)
    , m_pageNum(pageNum)
    , m_twoPages(twoPages)
    , pageSize(w, h)
    , estimated(true)
{
    m_image[0] = m_image[1] = NULL;
}

PageWidget::~PageWidget()
{
	deletePages();
}

QSize PageWidget::estimatedSize() const
{
    return estimated ? pageSize : getScaledSize();
}

void PageWidget::deletePages()
{
    for (int i=0; i<2; i++)
    {
        delete m_image[i];
        m_image[i] = NULL;
    }
}

void PageWidget::setImage(const Page &img1)
{
    deletePages();
    m_image[0] = new Page(img1);
    m_twoPages = false;
	estimated = false;
    redrawImages();
}

void PageWidget::setImage(const Page &img1, const Page &img2)
{
    deletePages();
    m_image[0] = new Page(img1);
    m_image[1] = new Page(img2);
    m_twoPages = true;
	estimated = false;
    redrawImages();
}

Page PageWidget::getPage(int n)
{
    if (n>=0 && n<2 && m_image[n])
    {
        return *m_image[n];
    }
    throw std::runtime_error("Invalid page index");
}

void PageWidget::dispose()
{
	ComicImageWidget::dispose();
    deletePages();
}
		
bool PageWidget::isDisposed() const
{
	return ComicImageWidget::isDisposed() || (m_image[0] == NULL);
}

void PageWidget::setEstimatedSize(int w, int h)
{
    if (estimated) // update size only if we have estimated size, otherwise we know last real size
    {
        if (pageSize.width() != w || pageSize.height() != h)
        {
            pageSize = QSize(w, h);
            redrawImages();
        }
    }
}

bool PageWidget::isEstimated() const
{
    return estimated;
}

int PageWidget::pageNumber() const
{
    return m_pageNum;
}

bool PageWidget::hasTwoPages() const
{
    return m_twoPages;
}

void PageWidget::drawPageNumber(int page, QPainter &p, int x, int y)
{
    const QString pagestr(QString::number(page + 1));
    const QFontMetrics mtr(p.fontMetrics());
    const int txtw(mtr.width(pagestr));
    p.setPen(Qt::black);
    p.fillRect(x - txtw - 5, y - 2 - mtr.height(), txtw + 5, mtr.height() + 4, Qt::white);
    p.drawText(x - txtw - 4, y - 4, pagestr);
}

void PageWidget::redraw(QPainter &p)
{
	const int pages = numOfPages();
	ViewProperties &props = view()->properties();

	if (pages == 1)
	{
		p.drawImage(0, 0, m_image[0]->getImage(), 0, 0);
		if (props.pageNumbers())
		{
			p.setWorldMatrixEnabled(false);
			drawPageNumber(m_image[0]->getNumber(), p, getScaledSize().width(), getScaledSize().height());
		}
	}
	else if (pages == 2)
	{
		const int swap(props.mangaMode());
		// clear areas not covered by page (if pages sizes differ)
		for (int i=0; i<2; i++)
		{
			const int j(i^swap);
			if (m_image[j]->height() < std::max(m_image[0]->height(), m_image[1]->height()))
			{
				p.fillRect(i*m_image[j]->width(), m_image[j]->height(), m_image[j]->width(), getSourceSize().height() - m_image[j]->height(), props.background());
				break; //only one page may be smaller
			}
		}

		p.drawImage(0, 0, m_image[0^swap]->getImage(), 0, 0);
		p.drawImage(m_image[0^swap]->width(), 0, m_image[1^swap]->getImage(), 0, 0);
		if (props.pageNumbers())
		{
			p.setWorldMatrixEnabled(false);
			drawPageNumber(std::max(m_image[swap]->getNumber(), m_image[1^swap]->getNumber()), p, getScaledSize().width(), getScaledSize().height());
		}
	}
}

ImageTransformJob* PageWidget::redrawJob()
{
    _DEBUG;
    PageRedrawJob *j = NULL;
    if (m_image[0])
    {
        j = new PageRedrawJob();
        j->setKey(JobKey(PAGE_REDRAW, m_image[0]->getNumber()));
        j->setSourceSize(getSourceSize());
        j->setViewProperties(view()->properties().getProperties());

        if (m_image[1])
        {
            j->setImage(m_image[0]->getImage(), m_image[1]->getImage());
        }
        else
        {
            j->setImage(m_image[0]->getImage());
        }
    }
    return j;
}

bool PageWidget::jobCompleted(const ImageJobResult &result)
{
    _DEBUG << result.key;
    if (m_image[0] && result.key.getKey() == m_image[0]->getNumber())
    {
        _DEBUG << "job for page" << m_image[0]->getNumber();
        if (ComicImageWidget::jobCompleted(result))
        {
            return true;
        }
    }
    return false;
}

void PageWidget::redrawImages()
{
    const int pages = numOfPages();

    int totalWidth, totalHeight;
    ViewProperties &props = view()->properties();

    if (pages == 0) // images not set or disposed, use last known or estimated size
    {
		totalWidth = pageSize.width();
		totalHeight = pageSize.height();
    }
    else if (pages == 1)
    {
		totalWidth = m_image[0]->width();
		totalHeight = m_image[0]->height();
    }
    else // 2 pages
    {
		totalWidth = m_image[0]->width() + m_image[1]->width();
		totalHeight = std::max(m_image[0]->height(), m_image[1]->height());
    }
    
	setSourceSize(totalWidth, totalHeight);
	if (!estimated)
	{
		redrawScaledImage();
	}
}

int PageWidget::numOfPages() const
{
    int n;
    for (n=0; n<2; n++)
    {
        if (!m_image[n])
            break;
    }
    return n;
}

void PageWidget::propsChanged()
{
    redrawImages();
} 
