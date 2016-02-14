/*
 * Modification History
 *
 * 2000-December-19		Jason Rohrer
 * Created.
 *
 * 2001-February-4		Jason Rohrer
 * Added key release functions.
 *
 * 2001-October-29		Jason Rohrer
 * Added fuction for querying a handler's focus.  
 *
 * 2008-October-27	Jason Rohrer
 * Switched to implementation-independent keycodes.
 */
 
 
#ifndef KEYBOARD_HANDLER_GL_INCLUDED
#define KEYBOARD_HANDLER_GL_INCLUDED 



// special key codes
#define  MG_KEY_F1                        0x0001
#define  MG_KEY_F2                        0x0002
#define  MG_KEY_F3                        0x0003
#define  MG_KEY_F4                        0x0004
#define  MG_KEY_F5                        0x0005
#define  MG_KEY_F6                        0x0006
#define  MG_KEY_F7                        0x0007
#define  MG_KEY_F8                        0x0008
#define  MG_KEY_F9                        0x0009
#define  MG_KEY_F10                       0x000A
#define  MG_KEY_F11                       0x000B
#define  MG_KEY_F12                       0x000C
#define  MG_KEY_LEFT                      0x0064
#define  MG_KEY_UP                        0x0065
#define  MG_KEY_RIGHT                     0x0066
#define  MG_KEY_DOWN                      0x0067
#define  MG_KEY_PAGE_UP                   0x0068
#define  MG_KEY_PAGE_DOWN                 0x0069
#define  MG_KEY_HOME                      0x006A
#define  MG_KEY_END                       0x006B
#define  MG_KEY_INSERT                    0x006C




/**
 * Interface for an object that can field OpenGL keystrokes.
 *
 * @author Jason Rohrer 
 */
class KeyboardHandlerGL { 
	
	public:

        virtual ~KeyboardHandlerGL() {
            }

		
		/**
		 * Gets whether this handler is focused (in other words,
		 * whether this handler wants to reserve keyboard
		 * events for itself).
		 *
		 * If no registered handler is focused, then all
		 * registered handlers receive keyboard events.  However,
		 * if some handlers are focused, then only focused handlers
		 * receive keyboard events.
		 *
		 * Note that in general, handlers should be unfocused.
		 * A default implementation is included in this interface,
		 * so handlers that do not care about focus can ignore
		 * this function.
		 *
		 * @return true iff this handler is focused. 
		 */
		virtual char isFocused();
		
		
		
		/**
		 * Callback function for when an ASCII-representable key is pressed.
		 *
		 * @param inKey ASCII representation of the pressed key.
		 * @param inX x position of mouse when key was pressed.
		 * @param inY y position of mouse when key was pressed.
		 */
		virtual void keyPressed( unsigned char inKey, int inX, int inY ) = 0;
		
		
		/**
		 * Callback function for when an ASCII-representable key is released.
		 *
		 * @param inKey ASCII representation of the released key.
		 * @param inX x position of mouse when key was released.
		 * @param inY y position of mouse when key was released.
		 */
		virtual void keyReleased( unsigned char inKey, int inX, int inY ) = 0;
		
		
		/**
		 * Callback function for when an special key is pressed.
		 *
		 * @param inKey integer constant representation of the pressed key.
		 * @param inX x position of mouse when key was pressed.
		 * @param inY y position of mouse when key was pressed.
		 */
		virtual void specialKeyPressed( int inKey, int inX, int inY ) = 0;
		
		
		/**
		 * Callback function for when an special key is released.
		 *
		 * @param inKey integer constant representation of the released key.
		 * @param inX x position of mouse when key was released.
		 * @param inY y position of mouse when key was released.
		 */
		virtual void specialKeyReleased( int inKey, int inX, int inY ) = 0;
		
	};



inline char KeyboardHandlerGL::isFocused() {
	return false;
	}



#endif
