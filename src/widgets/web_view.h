/*!
 * \file
 * \brief Cабклассинг QWebView, т.к. он криво масштабируется и установка дефолтного поведения
 */

#ifndef _avalon_web_view_h_
#define _avalon_web_view_h_

#include "sysheaders.h"

#ifndef AVALON_TEXT_BROWSER
	/*! 
	 * \brief Cабклассинг QWebPage для установки User-Agent ресурсов, загружаемых из сообщений
	 */
	class AWebPage : public QWebPage
	{
		public:

			AWebPage(QObject* parent = NULL) : QWebPage(parent) {}

		protected:

			QString userAgentForUrl (const QUrl& /*url*/) const { return getAgentString(); }
	};
#else
	/*! 
	 * \brief Совместимый объект страницы для сборок без Qt WebKit
	 */
	class AWebPage : public QObject
	{
		Q_OBJECT

		public:

			AWebPage(QObject* parent = NULL) : QObject(parent), m_network_manager() {}

			QNetworkAccessManager* networkAccessManager () { return &m_network_manager; }

		signals:

			void linkClicked (const QUrl& url);
			void linkHovered (const QString& link, const QString& title, const QString& text_content);

		private:

			QNetworkAccessManager m_network_manager;
	};
#endif

/*!
 * \brief Cабклассинг QWebView для установки дефолтного поведения
 */
class AWebView : public
#ifndef AVALON_TEXT_BROWSER
	QWebView
#else
	QTextBrowser
#endif
{
	Q_OBJECT

	public:

		AWebView  (QWidget* parent);
		~AWebView () {}

		QString selectedText () const;

#ifdef AVALON_TEXT_BROWSER
		AWebPage* page ();
#endif

		/*!
		 * \brief Флаг того, что мышь находится над ссылкой
		 * необходим для выдачи соответствующего QAction для возможности копирования ссылки, а не текста
		 * устанавливается в дочерних классах
		 */
		bool LinkHovered;

		/*!
		 * \brief Проверка возможности прокрутки страницы сообщения
		 * \return Флаг возможности скрола сообщения
		 */
		bool canScrollPage ();

		/*!
		 * \brief Прокрутка страницы сообщения
		 */
		void scrollPage ();

	protected:

		/*!
		 * \brief Кастомизация контекстного меню
		 */
		virtual void contextMenuEvent (QContextMenuEvent* event);

		/*!
		 * \brief Обработчик клавиш для реализации копирования по хоткею
		 */
		virtual void keyPressEvent (QKeyEvent* event);

	private slots:

		// меню
		void menu_yandex_triggered ();             /*!< \brief Поиск Яндекс         */
		void menu_wikipedia_triggered ();          /*!< \brief Поиск Википедии      */
		void menu_google_triggered ();             /*!< \brief Поиск Google         */
		void menu_google_translate_triggered ();   /*!< \brief Переводчик Google    */
		void menu_rsdn_triggered ();               /*!< \brief Поиск RSDN           */

#ifdef AVALON_TEXT_BROWSER
		void text_browser_link_hovered (const QString& link);
#endif

#ifdef AVALON_TEXT_BROWSER
	private:

		AWebPage* m_page;
#endif
};

#ifdef AVALON_TEXT_BROWSER
	inline AWebPage* AWebView::page () { return m_page; }
#endif

/*!
 * \brief Cабклассинг AWebView, т.к. он криво масштабируется
 */
class AWebViewWidget : public QFrame
{
	Q_OBJECT

	public:

		AWebViewWidget  (QWidget* parent);
		~AWebViewWidget ();

		/*!
		 * \brief Объект сабклассинга
		 */
		AWebView* View;

	protected:

		/*!
		 * \brief Масштабирование
		 */
		void resizeEvent (QResizeEvent* event);
};

#endif   // _avalon_web_view_h_
