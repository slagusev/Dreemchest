/**************************************************************************

 The MIT License (MIT)

 Copyright (c) 2015 Dmitry Sovetov

 https://github.com/dmsovetov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 **************************************************************************/

#include "NetworkApplication.h"
#include "Connection.h"

#include "../Connection/ConnectionMiddleware.h"

#include "../Sockets/UDPSocket.h"
#include "../Sockets/TCPSocket.h"

#include "../Packets/Ping.h"
#include "../Packets/RemoteCall.h"

DC_BEGIN_DREEMCHEST

namespace Network {

// ** Application::Application
Application::Application( void )
{
    DC_ABORT_IF( TypeInfo<Application>::name() != String( "Application" ), "the type info return an invalid name" );
    
#if DEV_DEPRECATED_PACKETS
	registerPacketHandler<packets::Ping>			  ( dcThisMethod( Application::handlePingPacket ) );
	registerPacketHandler<packets::KeepAlive>		  ( dcThisMethod( Application::handleKeepAlivePacket ) );
	registerPacketHandler<packets::Event>             ( dcThisMethod( Application::handleEventPacket ) );
	registerPacketHandler<packets::DetectServers>	  ( dcThisMethod( Application::handleDetectServersPacket ) );
	registerPacketHandler<packets::RemoteCall>        ( dcThisMethod( Application::handleRemoteCallPacket ) );
	registerPacketHandler<packets::RemoteCallResponse>( dcThisMethod( Application::handleRemoteCallResponsePacket ) );
#else
    addPacketHandler< PacketHandlerCallback<Packets::Event> >( dcThisMethod( Application::handleEventPacket ) );
    addPacketHandler< PacketHandlerCallback<Packets::Ping> >( dcThisMethod( Application::handlePingPacket ) );
    addPacketHandler< PacketHandlerCallback<Packets::RemoteCall> >( dcThisMethod( Application::handleRemoteCallPacket ) );
    addPacketHandler< PacketHandlerCallback<Packets::RemoteCallResponse> >( dcThisMethod( Application::handleRemoteCallResponsePacket ) );
#endif  /*  DEV_DEPRECATED_PACKETS  */
}

// ** Application::createConnection
ConnectionPtr Application::createConnection( TCPSocketWPtr socket )
{
	// Create the connection instance and add it to an active connections set
	ConnectionPtr connection( DC_NEW Connection( this, socket ) );
	m_connections.insert( connection );

    // Subscribe for connection events.
    connection->subscribe<Connection::Received>( dcThisMethod( Application::handlePacketReceived ) );
	connection->subscribe<Connection::Closed>( dcThisMethod( Application::handleConnectionClosed ) );

	// Setup the connection middleware
	connection->addMiddleware<PingInterval>( 500 );
	connection->addMiddleware<KeepAliveInterval>( 5000 );
	connection->addMiddleware<CloseOnTimeout>( 10000 );

	return connection;
}

// ** Application::removeConnection
void Application::removeConnection( ConnectionWPtr connection )
{
    // Unsubscribe from a connection events
    connection->unsubscribe<Connection::Received>( dcThisMethod( Application::handlePacketReceived ) );
	connection->unsubscribe<Connection::Closed>( dcThisMethod( Application::handleConnectionClosed ) );

    // Remove from a connections container
	m_connections.erase( connection );
}

// ** Application::eventListeners
ConnectionList Application::eventListeners( void ) const
{
	return ConnectionList();
}

#if DEV_DEPRECATED_PACKETS

// ** Application::handlePingPacket
bool Application::handlePingPacket( ConnectionPtr& connection, packets::Ping& packet )
{
	if( packet.iterations ) {
		connection->send<packets::Ping>( packet.iterations - 1, packet.timestamp, connection->time() );
	} else {
		u32 rtt  = connection->time() - packet.timestamp;
		u32 time = packet.time + rtt / 2;

		if( abs( ( s64 )time - connection->time() ) > 50 ) {
			LogWarning( "connection", "%dms time error detected\n", time - connection->time() );
			connection->setTime( time );
		}

		
		connection->setRoundTripTime( rtt );
	}
	
	return true;
}

// ** Application::handleKeepAlivePacket
bool Application::handleKeepAlivePacket( ConnectionPtr& connection, packets::KeepAlive& packet )
{
	connection->setTimeToLive( m_keepAliveTime );
	return true;
}

// ** Application::handleDetectServersPacket
bool Application::handleDetectServersPacket( ConnectionPtr& connection, packets::DetectServers& packet )
{
	return true;
}

// ** Application::handleEventPacket
bool Application::handleEventPacket( ConnectionPtr& connection, packets::Event& packet )
{
	// ** Find an event handler from this event id.
	EventHandlers::iterator i = m_eventHandlers.find( packet.eventId );

	if( i == m_eventHandlers.end() ) {
		LogWarning( "rpc", "unknown event %d received\n", packet.eventId );
		return false;
	}

	// ** Handle this event
	return i->second->handle( connection, packet );
}

// ** Application::handleRemoteCallPacket
bool Application::handleRemoteCallPacket( ConnectionPtr& connection, packets::RemoteCall& packet )
{
	if( i == m_remoteCallHandlers.end() ) {
	// ** Find a remote call handler
	RemoteCallHandlers::iterator i = m_remoteCallHandlers.find( packet.method );

		LogWarning( "rpc", "trying to invoke unknown remote procedure %d\n", packet.method );
		return false;
	}

	// ** Invoke a method
	return i->second->handle( connection, packet );
}

// ** Application::handleRemoteCallResponsePacket
bool Application::handleRemoteCallResponsePacket( ConnectionPtr& connection, packets::RemoteCallResponse& packet )
{
	return connection->handleResponse( packet );
}

#else

// ** Application::handlePingPacket
void Application::handlePingPacket( ConnectionWPtr connection, const Packets::Ping& ping )
{
	if( ping.iterations ) {
		connection->send<Packets::Ping>( ping.iterations - 1, ping.timestamp, connection->time() );
	} else {
		u32 rtt  = connection->time() - ping.timestamp;
		u32 time = ping.time + rtt / 2;

		if( abs( ( s64 )time - connection->time() ) > 50 ) {
			LogWarning( "connection", "%dms time error detected\n", time - connection->time() );
			connection->setTime( time );
		}

		
		connection->setRoundTripTime( rtt );
	}
}

// ** Application::handleRemoteCallPacket
void Application::handleRemoteCallPacket( ConnectionWPtr connection, const Packets::RemoteCall& packet )
{
	// Find a remote call handler
	RemoteCallHandlers::iterator i = m_remoteCallHandlers.find( packet.method );

	if( i == m_remoteCallHandlers.end() ) {
		LogWarning( "rpc", "trying to invoke unknown remote procedure %d\n", packet.method );
		return;
	}

	// Invoke a method
	i->second->handle( connection, packet );
}

// ** Application::handleRemoteCallResponsePacket
void Application::handleRemoteCallResponsePacket( ConnectionWPtr connection, const Packets::RemoteCallResponse& packet )
{
    connection->handleResponse( packet );
}

// ** Application::handleEventPacket
void Application::handleEventPacket( ConnectionWPtr connection, const Packets::Event& packet )
{
	// Find an event handler from this event id.
	EventHandlers::iterator i = m_eventHandlers.find( packet.eventId );

	if( i == m_eventHandlers.end() ) {
		LogWarning( "rpc", "unknown event %d received\n", packet.eventId );
		return;
	}

	// Handle this event
	i->second->handle( connection, packet );
}

#endif  /*  DEV_DEPRECATED_PACKETS  */

// ** Application::handlePacketReceived
void Application::handlePacketReceived( const Connection::Received& e )
{
#if DEV_DEPRECATED_PACKETS
    // Get the packet and connection from an event
    PacketUPtr    packet     = e.packet;
    ConnectionPtr connection = static_cast<Connection*>( e.sender.get() );

    // Find corresponding packet handler
	PacketHandlers::iterator j = m_packetHandlers.find( packet->typeId() );

    // No handler for this type of packet
	if( j == m_packetHandlers.end() ) {
		LogWarning( "packet", "unhandled packet of type %s received from %s\n", packet->typeName(), connection->address().toString() );
		return;
	}

    // Handle the packet
	if( !j->second->handle( connection, packet.get() ) ) {
		LogWarning( "packet", "malformed packet of type %s received from %s\n", packet->typeName(), connection->address().toString() );
	}
#else
    // Type cast the connection instance
    ConnectionWPtr connection = static_cast<Connection*>( e.sender.get() );

    // Create instance of a network packet
	PacketUPtr packet = m_packetFactory.construct( e.type );

    // The packet type is unknown - skip it
	if( packet == NULL ) {
        LogDebug( "packet", "packet of unknown type %d received, %d bytes skipped\n", e.type, e.packet->bytesAvailable() );
		return;
	}

    // Get the packet stream
    Io::ByteBufferWPtr stream = e.packet;

	// Read the packet data from a stream
    s32 position = stream->position();
	packet->deserialize( stream );
	s32 bytesRead = stream->position() - position;
    DC_BREAK_IF( bytesRead != e.size, "packet size mismatch" );

	// Find all handlers that are eligible to process this type of packet
    PacketHandlers::iterator i = m_packetHandlers.find( e.type );
    if( i == m_packetHandlers.end() ) {
        return;
    }

    for( PacketHandlerList::iterator j = i->second.begin(), end = i->second.end(); j != end; ++j ) {
        (*j)->process( connection, *packet );
    }
#endif  /*  DEV_DEPRECATED_PACKETS  */
}

// ** Application::handleConnectionClosed
void Application::handleConnectionClosed( const Connection::Closed& e )
{
	// Remove this connection from list
	removeConnection( static_cast<Connection*>( e.sender.get() ) );
}

// ** Application::update
void Application::update( u32 dt )
{
	// Update all connections
	ConnectionSet connections = m_connections;

	for( ConnectionSet::iterator i = connections.begin(); i != connections.end(); ++i ) {
		// Get the connection instance from an iterator
		ConnectionPtr connection = *i;

		// Update the connection instance
		connection->update( dt );

		// Close this connection if it was queued for closing
		if( connection->willBeClosed() ) {
			connection->close();
		}
	}
}

} // namespace Network

DC_END_DREEMCHEST