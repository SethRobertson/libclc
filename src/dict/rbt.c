/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static const char RCSid[] = "$Id: rbt.c,v 1.3 2002/07/18 22:52:46 dupuy Exp $";

#include "bstimpl.h"
#include "clchack.h"

/*
 * Transformation:
 *
 *        x                    y
 *       / \                  / \
 *      y   c     ===>       a   x
 *     / \                      / \
 *    a   b                    b   c
 */
PRIVATE void right_rotate(header_s *hp, tnode_s *x)
{
	tnode_s		*y	= LEFT( x ) ;
	tnode_s		*px	= PARENT( x ) ;
	tnode_s		*b	= RIGHT( y ) ;

#ifdef DEBUG_RBT
	if ( x == ANCHOR( hp ) )
		__dict_terminate( "RBT right_rotate", "x is the tree anchor" ) ;
#endif

	/*
	 * First fix 'y' and the tree above it. We need to:
	 *		a. Make x the right child of y
	 *		b. Determine if x was a left or right child and update the
	 *			corresponding field in x's parent.
	 */
	RIGHT( y ) = x ;
	PARENT( y ) = px ;
	if ( x == LEFT( px ) )
		LEFT( px ) = y ;
	else
		RIGHT( px ) = y ;
	
	/*
	 * Now fix the tree below 'y'. We need to:
	 *		a. Fix the parent of 'x'
	 *		b. Change the left child of 'x' to 'b'
	 *		c. Change the parent of 'b' to 'x'
	 */
	PARENT( x ) = y ;
	LEFT( x ) = b ;
	PARENT( b ) = x ;
}



/*
 * Transformation:
 *
 *        x                  y
 *       / \                / \
 *      a   y     ===>     x   c
 *         / \            / \
 *        b   c          a   b
 */
PRIVATE void left_rotate(header_s *hp, tnode_s *x)
{
	tnode_s		*y	= RIGHT( x ) ;
	tnode_s		*px	= PARENT( x ) ;
	tnode_s		*b	= LEFT( y ) ;


#ifdef DEBUG_RBT
	if ( x == ANCHOR( hp ) )
		__dict_terminate( "RBT left_rotate", "x is the tree anchor" ) ;
#endif

	/*
	 * First fix 'y' and the tree above it. We need to:
	 *		a. Make x the left child of y
	 *		b. Determine if x was a left or right child and update the
	 *			corresponding field in x's parent.
	 */
	LEFT( y ) = x ;
	PARENT( y ) = px ;
	if ( x == LEFT( px ) )
		LEFT( px ) = y ;
	else
		RIGHT( px ) = y ;
	
	/*
	 * Now fix the tree below 'y'. We need to:
	 *    a. Fix the parent of 'x'
	 *    b. Change the right child of 'x' to 'b'
	 *    c. Change the parent of 'b' to 'x'
	 */
	PARENT( x ) = y ;
	RIGHT( x ) = b ;
	PARENT( b ) = x ;
}


/*
 * This function is called after node 'new' is inserted in the tree.
 * The function makes sure that the red-black properties of the tree
 * are not violated by the insertion
 */
void __dict_rbt_insfix(header_s *hp, tnode_s *newnode)
{
	register tnode_s	*x, *y ;
	register tnode_s	*px ;
	tnode_s			*ppx ;

	x = newnode ;
	COLOR( x ) = RED ;

	/*
	 * Note that the color of the anchor is BLACK so the loop will
	 * always stop there with x equal to the root of the tree.
	 */
	while ( COLOR( px = PARENT( x ) ) == RED )
	{
		ppx = PARENT( px ) ;
		if ( px == LEFT( ppx ) )
		{
			y = RIGHT( ppx ) ;					/* y is px's sibling and x's uncle */
			if ( COLOR( y ) == RED )			/* both px and y are red */
			{
				COLOR( px ) = BLACK ;
				COLOR( y ) = BLACK ;
				COLOR( ppx ) = RED ;				/* grandparent of x gets red */
				x = ppx ;							/* advance x to its grandparent */
			}
			else
			{
				if ( x == RIGHT( px ) )
					left_rotate( hp, x = px ) ;	/* notice the assignment */
				/*
				 * px cannot be used anymore because of the possible left rotation
				 * ppx however is still valid
				 */
				COLOR( PARENT( x ) ) = BLACK ;
				COLOR( ppx ) = RED ;
				right_rotate( hp, ppx ) ;
			}
		}
		else		/* px == RIGHT( ppx ) */
		{
			/*
			 * XXX:	The else clause is symmetrical with the 'then' clause
			 *    	(RIGHT is replaced by LEFT and vice versa). We should
			 * 		be able to use the same code.
			 */
			y = LEFT( ppx ) ;
			if ( COLOR( y ) == RED )
			{
				COLOR( px ) = BLACK ;
				COLOR( y ) = BLACK ;
				COLOR( ppx ) = RED ;
				x = ppx ;
			}
			else
			{
				if ( x == LEFT( px ) )
					right_rotate( hp, x = px ) ;
				COLOR( PARENT( x ) ) = BLACK ;
				COLOR( ppx ) = RED ;
				left_rotate( hp, ppx ) ;
			}
		}
	}
	COLOR( ROOT( hp ) ) = BLACK ;
}


void __dict_rbt_delfix(header_s *hp, tnode_s *x)
{
	tnode_s		*px ;			/* parent of x */
	tnode_s		*sx ;			/* sibling of x */

	/*
	 * On exit from the loop x will be either the ROOT or a RED node
	 */
	while ( x != ROOT( hp ) && COLOR( x ) == BLACK )
	{
		px = PARENT( x ) ;
		if ( x == LEFT( px ) )
		{
			sx = RIGHT( px ) ;
			if ( COLOR( sx ) == RED )
			{
				COLOR( sx ) = BLACK ;
				COLOR( px ) = RED ;
				left_rotate( hp, px ) ;
				sx = RIGHT( px ) ;
			}

			/*
			 * Now sx is BLACK
			 */
			if ( COLOR( LEFT( sx ) ) == BLACK && COLOR( RIGHT( sx ) ) == BLACK )
			{
				COLOR( sx ) = RED ;
				x = px ;										/* move up the tree */
			}
			else
			{
				if ( COLOR( RIGHT( sx ) ) == BLACK )
				{
					COLOR( LEFT( sx ) ) = BLACK ;		/* was RED */
					COLOR( sx ) = RED ;
					right_rotate( hp, sx ) ;
					sx = RIGHT( px ) ;
				}

				/*
				 * Now RIGHT( sx ) is RED and sx is BLACK
				 */
				COLOR( sx ) = COLOR( px ) ;
				COLOR( px ) = BLACK ;
				COLOR( RIGHT( sx ) ) = BLACK ;
				left_rotate( hp, px ) ;
				x = ROOT( hp ) ;						/* exit the loop */
			}
		}
		else
		{
			sx = LEFT( px ) ;
			if ( COLOR( sx ) == RED )
			{
				COLOR( sx ) = BLACK ;
				COLOR( px ) = RED ;
				right_rotate( hp, px ) ;
				sx = LEFT( px ) ;
			}

			if ( COLOR( LEFT( sx ) ) == BLACK && COLOR( RIGHT( sx ) ) == BLACK )
			{
				COLOR( sx ) = RED ;
				x = px ;
			}
			else
			{
				if ( COLOR( LEFT( sx ) ) == BLACK )
				{
					COLOR( RIGHT( sx ) ) = BLACK ;
					COLOR( sx ) = RED ;
					left_rotate( hp, sx ) ;
					sx = LEFT( px ) ;
				}

				COLOR( sx ) = COLOR( px ) ;
				COLOR( px ) = BLACK ;
				COLOR( LEFT( sx ) ) = BLACK ;
				right_rotate( hp, px ) ;
				x = ROOT( hp ) ;
			}
		}
	}
	COLOR( x ) = BLACK ;
}

